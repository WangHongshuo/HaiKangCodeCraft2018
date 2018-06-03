#include "UAVAI.h"

UAVAI::UAVAI()
{
	tmpPath.reserve(600);
	minPath.reserve(600);
	tmpGoodsPath.reserve(600);
	minGoodsPath.reserve(600);
	availablePoints.reserve(11);
	dodgePosition.reserve(11);
	tmpUavMoveScope_1.reserve(11);
	tmpUavMoveScope_2.reserve(11);
	tmpUavMoveScope_3.reserve(11);
	MOVE_DIRECTION_DELTA.resize(10);
	MOVE_DIRECTION_DELTA = { Point3(-1,-1,0), Point3(-1,0,0), Point3(-1,1,0), 
		                     Point3(0,-1,0),  Point3(0,1,0),  Point3(1,-1,0),  
		                     Point3(1,0,0),   Point3(1,1,0),  Point3(0,0,-1), 
		                     Point3(0,0,1) };
}

UAVAI::~UAVAI()
{
	map = NULL;
	match = NULL;
	plan = NULL;
}

void UAVAI::initPtr(MAP_INFO *_map, MATCH_STATUS *_match, FLAY_PLANE *_flayPlane)
{
	map = _map;
	match = _match;
	plan = _flayPlane;
	isInitPtr = true;
	UAVNum = match->nUavWeNum;
	UAVAliveNum = match->nUavWeNum;
	// init uav data
	for (int i = 0; i < MAX_UAV_NUM; i++)
	{
		match->astWeUav[i].nGoodsTarget = -1;
		match->astWeUav[i].nPath.reserve(MAX_PATH_LENGTH);
		match->astWeUav[i].nLastPosMapMark = AREA_OBJ::IS_NULL;
		match->astWeUav[i].nCurrentPathIndex = -1;
		match->astWeUav[i].nAttackTarget = -1;
	}
	goodsStatus.resize(MAX_GOODS_NUM);
	for (int i = 0; i < MAX_GOODS_NUM; i++)
	{
		goodsStatus[i].nIsRejectUav.resize(MAX_UAV_NUM);
	}
	uavNo.reserve(10);
	initUavValue();
	cheapestUavIndex = 0;
	mostExpensiveUavIndex = 0;
	for (int i = 1; i < map->nUavPriceNum; i++)
	{
		if (map->astUavPrice[i].nValue < map->astUavPrice[cheapestUavIndex].nValue)
			cheapestUavIndex = i;
		if (map->astUavPrice[i].nValue > map->astUavPrice[mostExpensiveUavIndex].nValue)
			mostExpensiveUavIndex = i;
	}
	MAX_ALIVE_UAV_NUM = map->nMapX / 2;
	MAX_ATTACKER_UAV_NUM = 1;
}

void UAVAI::initMap()
{
	Point3 _p1, _p2;
	// init mapArray
	pathMap.resize(map->nMapX);
	for (int i = 0; i < map->nMapX; i++)
	{
		pathMap[i].resize(map->nMapY);
		for (int j = 0; j < map->nMapY; j++)
		{
			pathMap[i][j].resize(map->nMapZ);
			pathMap[i][j].assign(map->nMapZ, AREA_OBJ::IS_NULL);
		}
	}
	// init fog in the mapArray
	for (int i = 0; i < map->nFogNum; i++)
	{
		_p1.setPoint(map->astFog[i].nX, map->astFog[i].nY, map->astFog[i].nB);
		_p2 = _p1 + Point3(map->astFog[i].nL - 1, map->astFog[i].nW - 1, 0);
		_p2.z = map->astFog[i].nT;
		fillArea(pathMap, _p1, _p2, AREA_OBJ::IS_FOG);
	}
	// init building in the mapArray
	for (int i = 0; i < map->nBuildingNum; i++)
	{
		_p1.setPoint(map->astBuilding[i].nX, map->astBuilding[i].nY, 0);
		_p2 = _p1 + Point3(map->astBuilding[i].nL - 1, map->astBuilding[i].nW - 1, map->astBuilding[i].nH - 1);
		fillArea(pathMap, _p1, _p2, AREA_OBJ::IS_BUILDING);
	}
	statusMap = pathMap;
	restoredMap = pathMap;
}

void UAVAI::setInitUavTarget()
{
	// set init target as the center of map (whatever the target obj is)
	Point3 _center, _tP;
	_center.setPoint(map->nMapX / 2, map->nMapY / 2, (map->nHLow + map->nHHigh) / 2);
	while (getMapValue(statusMap, _center) <= -4)
	{
		_center.z += 1;
		if (_center.z >= map->nHHigh)
			break;
	}
	while (getMapValue(statusMap, _center) <= -4)
	{
		_center.x += 1;
		if (_center.x >= map->nMapX - 1)
			break;
	}
	match->astWeUav[0].nTarget = _center;
	getPath(match->astWeUav[0]);
	match->astWeUav[0].nAction = UAV_ACTION::UAV_MOVING;
	int _tmp = 0;
	for (int i = match->nUavWeNum-1; i >= 0; i--)
	{
		match->astWeUav[i].nPathLength = map->nHLow + _tmp;
		match->astWeUav[i].nPath = match->astWeUav[0].nPath;
		match->astWeUav[i].nPath.resize(match->astWeUav[i].nPathLength);
		match->astWeUav[i].nIsGetPath = true;
		match->astWeUav[i].nAction = UAV_ACTION::UAV_MOVING;
		match->astWeUav[i].nTarget = match->astWeUav[i].nPath[match->astWeUav[i].nPathLength - 1];
		_tmp++;
	}
	for (int i = 0; i < match->nUavWeNum; i++)
	{
		if (!strcmp(match->astWeUav[i].szType, map->astUavPrice[cheapestUavIndex].szType))
		{
			match->astWeUav[i].nAction = UAV_ACTION::UAV_ATTACK;
			UAVAttackerNum++;
			break;
		}
			
	}
}

void UAVAI::getNextAction()
{
	UAVValues = 0;
	// reset uav.isMoved flag
	resetUavMovedFlag();

	// init uav value
	initUavValue();
	// init uav nLastPos
	if (match->nTime == 1)
	{
		initUavLastPos();
		enemyParkingPos = match->astEnemyUav[0].nPos;
		enemyParkingPos.z = 0;
	}

	// set enemy uav pos marks
	for (int i = 0; i < match->nUavEnemyNum; i++)
	{
		if (match->astEnemyUav[i].nStatus == UAV_STATUS::UAV_FOG)
			continue;
		setMapValue(statusMap, match->astEnemyUav[i].nPos, match->astEnemyUav[i].nNO + 1000);
	}

	// check uav is alive or not
	UAVNum = match->nUavWeNum;
	for (int i = 0; i < match->nUavWeNum; i++)
	{
		if (match->astWeUav[i].nIsCrash)
			continue;
		if (match->astWeUav[i].nStatus == UAV_STATUS::UAV_CRASH)
		{
			match->astWeUav[i].nIsCrash = true;
			setMapValue(statusMap, match->astWeUav[i].nPos, match->astWeUav[i].nLastPosMapMark);
			continue;
		}
	}
	// check uav attacker
	UAVAttackerNum = 0;
	for (int i = 0; i < match->nUavWeNum; i++)
	{
		if (match->astWeUav[i].nIsCrash)
			continue;
		if (match->astWeUav[i].nAction == UAV_ACTION::UAV_ATTACK)
			UAVAttackerNum++;
	}

	setAttackTarget();
	updateAttackTarget();
	// search goods
	searchGoods();

	// get next moving step and copy to plan
	money = match->nWeValue;
	moveAllUavByAction(UAV_ACTION::UAV_CATCHING, UAVNum);
	moveAllUavByAction(UAV_ACTION::UAV_DELIVERYING, UAVNum);
	moveAllUavByAction(UAV_ACTION::UAV_MOVING, UAVNum);
	moveAllUavByAction(UAV_ACTION::UAV_ATTACK, UAVNum);
	moveAllUavByAction(UAV_ACTION::UAV_STANDBY, UAVNum);
	UAVAliveNum = 0;
	for (int i = 0; i < match->nUavWeNum; i++)
	{
		if (match->astWeUav[i].nIsCrash)
			continue;
		UAVValues += match->astWeUav[i].nValue;
		copyUav(match->astWeUav[i], plan->astUav[UAVAliveNum]);
		UAVAliveNum++;
	}
	cout << "Available money: " << money << " " << "Uav values: " << UAVValues << endl;
	cout << "Value Counts: " << money + UAVValues << endl;
	plan->nUavNum = UAVAliveNum;

	buyNewUav();

	// remove enemy uav pos marks
	for (int i = 0; i < match->nUavEnemyNum; i++)
	{
		if (match->astEnemyUav[i].nStatus == UAV_STATUS::UAV_FOG)
			continue;
		setMapValue(statusMap, match->astEnemyUav[i].nPos, AREA_OBJ::IS_NULL);
	}
}

void UAVAI::initUavLastPos()
{
	for (int i = 0; i < MAX_UAV_NUM; i++)
	{
		match->astWeUav[i].nLastPos = match->astWeUav[0].nPos;
	}
}

void UAVAI::copyUav(const UAV & _src, UAV & _dst)
{
	_dst.nNO = _src.nNO;
	strcpy(_dst.szType, _src.szType);
	_dst.nPos = _src.nPos;
	_dst.nLoadWeight = _src.nLoadWeight;
	_dst.nStatus = _src.nStatus;
	_dst.nGoodsNo = _src.nGoodsNo;
}

int UAVAI::getMapValue(const vector<vector<vector<int>>>& _array, const Point3 & _p)
{
	return _array[_p.x][_p.y][_p.z];
}

void UAVAI::setMapValue(vector<vector<vector<int>>>& _array, const Point3 & _p, int _v)
{
	_array[_p.x][_p.y][_p.z] = _v;
}

void UAVAI::fillArea(vector<vector<vector<int>>>& _Array, const Point3 & _p1, const Point3 & _p2, int _fill)
{
	for (int i = _p1.x; i <= _p2.x; i++)
		for (int j = _p1.y; j <= _p2.y; j++)
			for (int k = _p1.z; k <= _p2.z; k++)
				_Array[i][j][k] = _fill;
}

void UAVAI::resetUavMovedFlag()
{
	for (int i = 0; i < match->nUavWeNum; i++)
	{
		match->astWeUav[i].nIsMoved = false;
		match->astWeUav[i].nIsUpdateMapMark = false;
	}
}

void UAVAI::moving(UAV & _uav)
{
	moveAction = -1;
	// save uav.nPos
	tmpPoint_1 = _uav.nPos;
	_uav.nIsMoved = true;
	// if uav.action is standby, check environment
	if (_uav.nAction == UAV_ACTION::UAV_STANDBY)
	{
		environmentAware(_uav);
		return;
	}
	// check path
	if (!_uav.nIsGetPath)
	{
		return;
	}
	// path index +1
	_uav.nCurrentPathIndex++;
	// check pos
	if (_uav.nCurrentPathIndex < _uav.nPathLength - 1)
	{
		moveAction = environmentAware(_uav);
		switch (moveAction)
		{
		case MOVE_ACTION::M_NORMAL:
		{
			// move to next position
			_uav.nPos = _uav.nPath[_uav.nCurrentPathIndex];
			break;
		}
		case MOVE_ACTION::M_STANDBY:
		{
			_uav.nCurrentPathIndex--;
			break;
		}
		case MOVE_ACTION::M_NEWPATH:
		default:
			break;
		}

		// check goods status
		if (_uav.nAction == UAV_ACTION::UAV_CATCHING)
		{
			for (int i = 0; i < match->nGoodsNum; i++)
			{
				if (match->astGoods[i].nNO == _uav.nGoodsTarget)
				{
					tmpPoint_2 = match->astGoods[i].nStartPos;
					tmpPoint_2.z = map->nHLow - 1;
					if (match->astGoods[i].nState != 0 || 
					   (getMapValue(statusMap, tmpPoint_2) >= 1000 && _uav.nPos.z >= map->nHLow) ||
					    match->astGoods[i].nLeftTime < _uav.nPathLength - _uav.nCurrentPathIndex)
					{
						goodsStatus[match->astGoods[i].nNO].nIsRejectUav[_uav.nNO] = true;
						tmpPoint_2.z = map->nHLow + 2;
						clearUavPath(_uav);
						_uav.nTarget = tmpPoint_2;
						getPath(_uav);
						updateWeUavMark(_uav);
						return;
					}
					break;
				}
			}
		}
	}
	else if (_uav.nCurrentPathIndex == _uav.nPathLength - 1)
	{
		// will be locked
		if (getMapValue(statusMap, _uav.nPath[_uav.nCurrentPathIndex]) >= 0)
		{
			_uav.nCurrentPathIndex--;
			return;
		}
		// move to next position
		_uav.nPos = _uav.nPath[_uav.nCurrentPathIndex];
		if (_uav.nAction == UAV_ACTION::UAV_CATCHING)
		{
			for (int i = 0; i < match->nGoodsNum; i++)
			{
				if (match->astGoods[i].nNO == _uav.nGoodsTarget)
				{
					match->astGoods[i].nState = 1;
					_uav.nGoodsNo = _uav.nGoodsTarget;
					clearUavPath(_uav);
					_uav.nTarget = match->astGoods[i].nEndPos;
					getPath(_uav);
					_uav.nAction = UAV_ACTION::UAV_DELIVERYING;
					break;
				}
			}
		}
		else if (_uav.nAction == UAV_ACTION::UAV_DELIVERYING)
		{
			clearUavPath(_uav);
			_uav.nTarget.setPoint(_uav.nPos);
			_uav.nTarget.z = map->nHLow + 2;
			getPath(_uav);
			_uav.nAction = UAV_ACTION::UAV_MOVING;
			for (int i = 0; i < MAX_GOODS_NUM; i++)
			{
				goodsStatus[i].nIsRejectUav[_uav.nNO] = false;
			}
		}
		else
		{
			clearUavPath(_uav);
		}
	}
	else
	{
		clearUavPath(_uav);
	}
	// if the position changed, update map marks
	if (tmpPoint_1 != _uav.nPos && moveAction != MOVE_ACTION::M_NEWPATH)
	{
		updateWeUavMark(_uav);
	}
}

void UAVAI::moveAllUavByAction(UAV_ACTION _action, int &_uavNum)
{
	for (int i = 0; i < _uavNum; i++)
	{
		if (match->astWeUav[i].nIsCrash)
			continue;
		if (match->astWeUav[i].nIsMoved)
			continue;
		if (match->astWeUav[i].nAction == _action)
			moving(match->astWeUav[i]);
		else
			continue;
	}
}

void UAVAI::updateWeUavMark(UAV & _uav)
{
	// if the position had changed
	if (_uav.nPos != _uav.nLastPos)
	{
		tmpMark_2 = getMapValue(statusMap, _uav.nLastPos);
		if (tmpMark_2 == _uav.nNO)
		{
			tmpMark_2 = getMapValue(restoredMap, _uav.nLastPos);
			setMapValue(statusMap, _uav.nLastPos, tmpMark_2);
		}
		setMapValue(statusMap, _uav.nPos, _uav.nNO);
		_uav.nLastPos = _uav.nPos;
	}
}

int UAVAI::getMoveDirection(UAV & _uav)
{
	if(_uav.nPos.z == _uav.nPath[_uav.nCurrentPathIndex].z)
		return MOVE_DIRECTION::M_HORIZONTAL;
	else
	{
		if (_uav.nPos.z < _uav.nPath[_uav.nCurrentPathIndex].z)
			return MOVE_DIRECTION::M_UPWARD;
		else
			return MOVE_DIRECTION::M_DOWNWARD;
	}
}

int UAVAI::isUavInArea(const Point3 & _p, vector<int>& _uavNo)
{
	_uavNo.resize(0);
	tmpUavMoveScope_1.resize(0);
	int _No = -1;
	int _num = 0;
	getUavMoveScope(_p, tmpUavMoveScope_1);
	for (int i = 0; i < int(tmpUavMoveScope_1.size()); i++)
	{
		_No = getMapValue(statusMap, tmpUavMoveScope_1[i]);
		if (_No >= 0)
		{
			_uavNo.push_back(_No);
			_num++;
		}
	}
	return _num;
}

bool UAVAI::isPositionInMap(const Point3 & _p)
{
	if(_p.x < 0 || _p.x > map->nMapX-1)
		return false;
	if (_p.y < 0 || _p.y > map->nMapY - 1)
		return false;
	if (_p.z < 0 || _p.z > map->nMapZ - 1)
		return false;
	return true;
}

Point3 UAVAI::getBestDodgePosition(const Point3 & _p, UAV & _uav)
{

	return Point3();
}

Point3 UAVAI::getAvailableAreaPosisiton(const Point3 & _p, UAV &_uav)
{
	Point3 _tmp = _p;
	availablePoints.resize(0);
	int _tmpMark;
	if (_p.z >= map->nHLow)
	{
		for (int i = -1; i <= 1; i++)
		{
			for (int j = -1; j <= 1; j++)
			{
				_tmp = _p;
				_tmp.x += i;
				_tmp.y += j;
				if (_tmp == _p)
					continue;
				if (_tmp == _uav.nLastPos)
					continue;
				if (!isPositionInMap(_tmp))
					continue;
				_tmpMark = getMapValue(statusMap, _tmp);
				if (_tmpMark == AREA_OBJ::IS_NULL || _tmpMark == AREA_OBJ::IS_FOG)
					availablePoints.push_back(_tmp);
			}
		}
	}
	_tmp = _p;
	_tmp.z += 1;
	if (_tmp.z <= map->nHHigh && _tmp != _uav.nLastPos)
	{
		_tmpMark = getMapValue(statusMap, _tmp);
		if (_tmpMark == AREA_OBJ::IS_NULL || _tmpMark == AREA_OBJ::IS_FOG)
			availablePoints.push_back(_tmp);
	}
	_tmp.z -= 2;
	if (_tmp.z >= 0 && _tmp != _uav.nLastPos)
	{
		_tmpMark = getMapValue(statusMap, _tmp);
		if (_tmpMark == AREA_OBJ::IS_NULL || _tmpMark == AREA_OBJ::IS_FOG)
			availablePoints.push_back(_tmp);
	}
	if (availablePoints.size() == 0)
	{
		return _p;
	}
	else
	{
		srand(time(0));
		return availablePoints[rand() % (availablePoints.size())];
	}
}

void UAVAI::uavDodgeAndGetNewPath(UAV & _uav, Point3 & _dodgePosition)
{
	_uav.nPos = _dodgePosition;
	_uav.nCurrentPathIndex = -1;
	_uav.nPath.resize(0);
	_uav.nPathLength = 0;
	getPath(_uav.nPos, _uav.nTarget, _uav.nPath, _uav.nPathLength);
	updateWeUavMark(_uav);
}

int UAVAI::getEnemyUavIndexByNo(const int & _No)
{
	for (int i = 0; i < match->nUavEnemyNum; i++)
	{
		if (match->astEnemyUav[i].nNO == _No)
			return i;
	}
	return -1;
}

void UAVAI::getUavMoveScope(const Point3 & _center, vector<Point3>& _scope)
{
	int _tmpMark;
	if(!_scope.empty())
		_scope.resize(0);
	for (int i = 0; i < MOVE_DIRECTION_NUM; i++)
	{
		tmpPoint_5 = _center + MOVE_DIRECTION_DELTA[i];
		if (!isPositionInMap(tmpPoint_5))
			continue;
		_tmpMark = getMapValue(statusMap, tmpPoint_5);
		if (_tmpMark == AREA_OBJ::IS_BUILDING)
			continue;
		_scope.push_back(tmpPoint_5);
	}
}

int UAVAI::environmentAware(UAV & _uav)
{
	int _uavNum = 0;
	if (_uav.nAction == UAV_ACTION::UAV_STANDBY)
	{
		_uavNum = isUavInArea(_uav.nPos, uavNo);
		if (_uavNum > 1)
		{
			for (int i = 0; i < _uavNum; i++)
			{
				// if there is enemy uav exist
				if (uavNo[i] >= 1000)
				{
					if (_uav.nPos.z < map->nHLow)
					{
						return -1;
					}
					// if enemy and ally in the same height
					if (match->astEnemyUav[uavNo[i]%1000].nPos.z == _uav.nPos.z)
					{
						if (_uav.nPos.z + 1 <= map->nHHigh)
						{
							tmpPoint_4 = _uav.nPos;
							tmpPoint_4.z += 1;
							tmpMark_1 = getMapValue(statusMap, tmpPoint_4);
							if (tmpMark_1 == AREA_OBJ::IS_NULL || tmpMark_1 == AREA_OBJ::IS_FOG)
							{
								_uav.nPos.z += 1;
								updateWeUavMark(_uav);
								return -1;
							}
							tmpPoint_4.z -= 2;
							if (tmpPoint_4.z >= 0)
							{
								tmpMark_1 = getMapValue(statusMap, tmpPoint_4);
								if (tmpMark_1 == AREA_OBJ::IS_NULL || tmpMark_1 == AREA_OBJ::IS_FOG)
								{
									_uav.nPos.z -= 1;
									updateWeUavMark(_uav);
									return -1;
								}
							}
						}
						else
						{
							tmpPoint_4 = getAvailableAreaPosisiton(_uav.nPos, _uav);
							_uav.nPos = tmpPoint_4;
							updateWeUavMark(_uav);
							return -1;
						}
					}
					else
					{
						tmpPoint_4 = getAvailableAreaPosisiton(_uav.nPos, _uav);
						_uav.nPos = tmpPoint_4;
						updateWeUavMark(_uav);
						return -1;
					}
				}
			}
			return -1;
		}
		return -1;
	}
	tmpMark_1 = getMapValue(statusMap, _uav.nPath[_uav.nCurrentPathIndex]);
	uavMoveDirection = getMoveDirection(_uav);
	// if is enemy
	if (tmpMark_1 >= 1000)
	{
		if (uavMoveDirection == MOVE_DIRECTION::M_DOWNWARD || uavMoveDirection == MOVE_DIRECTION::M_UPWARD)
		{
			return MOVE_ACTION::M_STANDBY;
		}
		else if (_uav.nPos.z < map->nHHigh)
		{
			tmpPoint_4 = _uav.nPos;
			tmpPoint_4.z += 1;
			uavDodgeAndGetNewPath(_uav, tmpPoint_4);
			return MOVE_ACTION::M_NEWPATH;
		}
		else
		{
			// unknow
			return MOVE_ACTION::M_STANDBY;
		}
	}
	// if is ally
	else if (tmpMark_1 >= 0)
	{
		uavAlly = &(match->astWeUav[tmpMark_1]);
		if (uavMoveDirection == MOVE_DIRECTION::M_UPWARD)
		{
			if (uavAlly->nAction == UAV_ACTION::UAV_STANDBY)
			{
				if (uavAlly->nPos.z < map->nHLow)
				{
					tmpPoint_4 = _uav.nPos;
					tmpPoint_4.z += 1;
					uavDodgeAndGetNewPath(_uav, tmpPoint_4);
					uavAlly->nPos.z += 1;
					uavAlly->nIsMoved = true;
					updateWeUavMark(*uavAlly);
					return MOVE_ACTION::M_NEWPATH;
				}
				else
				{
					tmpPoint_4 = getAvailableAreaPosisiton(uavAlly->nPos, *uavAlly);
					uavAlly->nPos = tmpPoint_4;
					uavAlly->nIsMoved = true;
					updateWeUavMark(*uavAlly);
					return MOVE_ACTION::M_NORMAL;
				}
			}
			else
			{
				// there is something wrong in taking off part
				if (!uavAlly->nIsMoved)
				{
					if (uavAlly->nPath[uavAlly->nCurrentPathIndex + 1] == _uav.nPos  && 
						uavAlly->nPos == _uav.nPath[_uav.nCurrentPathIndex])
					{
						uavAlly->nIsMoved = true;
						tmpPoint_4 = getAvailableAreaPosisiton(uavAlly->nPos, *uavAlly);
						uavDodgeAndGetNewPath(*uavAlly, tmpPoint_4);
						return MOVE_ACTION::M_NORMAL;
					}
					else
					{
						return MOVE_ACTION::M_NORMAL;
					}
				}
				else
				{
					return MOVE_ACTION::M_STANDBY;
				}
			}
		}
		else if (uavMoveDirection == MOVE_DIRECTION::M_DOWNWARD)
		{
			if (uavAlly->nAction == UAV_ACTION::UAV_STANDBY)
			{
				if (uavAlly->nPos.z < map->nHLow)
				{
					// maybe something wrong
					tmpPoint_4 = _uav.nPos;
					tmpPoint_4.z += 1;
					uavDodgeAndGetNewPath(_uav, tmpPoint_4);
					uavAlly->nPos.z += 1;
					updateWeUavMark(*uavAlly);
					return MOVE_ACTION::M_NEWPATH;
				}
				else
				{
					tmpPoint_4 = getAvailableAreaPosisiton(uavAlly->nPos, *uavAlly);
					// if can't find dodge position, upward
					if (tmpPoint_4 == uavAlly->nPos)
					{
						tmpPoint_4 = _uav.nPos;
						tmpPoint_4.z += 1;
						uavDodgeAndGetNewPath(_uav, tmpPoint_4);
						uavAlly->nPos.z += 1;
						updateWeUavMark(*uavAlly);
						return MOVE_ACTION::M_NEWPATH;
					}
					else
					{
						uavAlly->nPos = tmpPoint_4;
						updateWeUavMark(*uavAlly);
						return MOVE_ACTION::M_NORMAL;
					}
				}
			}
			else
			{
				tmpPoint_4 = _uav.nPos;
				tmpPoint_4.z += 1;
				uavDodgeAndGetNewPath(_uav, tmpPoint_4);
				return MOVE_ACTION::M_NEWPATH;
			}
		}
		else
		{
			if (uavAlly->nAction == UAV_ACTION::UAV_STANDBY)
			{
				if (uavAlly->nPos.z < map->nHHigh)
				{
					uavAlly->nPos.z += 1;
					uavAlly->nIsMoved = true;
					updateWeUavMark(*uavAlly);
				}
				else
				{
					tmpPoint_4 = getAvailableAreaPosisiton(uavAlly->nPos, *uavAlly);
					uavAlly->nPos = tmpPoint_4;
					uavAlly->nIsMoved = true;
					updateWeUavMark(*uavAlly);
				}
				return MOVE_ACTION::M_NORMAL;
			}
			else if (uavAlly->nAction == UAV_ACTION::UAV_ATTACK)
			{
				if (uavAlly->nPathLength == 0)
				{
					tmpPoint_4 = getAvailableAreaPosisiton(uavAlly->nPos, *uavAlly);
					uavAlly->nPos = tmpPoint_4;
					uavAlly->nIsMoved = true;
					updateWeUavMark(*uavAlly);
					return MOVE_ACTION::M_NORMAL;
				}
				else
				{
					return MOVE_ACTION::M_STANDBY;
				}
			}
			else
			{
				tmpPoint_4 = getAvailableAreaPosisiton(_uav.nPos, _uav);
				uavDodgeAndGetNewPath(_uav, tmpPoint_4);
				return MOVE_ACTION::M_NEWPATH;
			}
		}
		
	}
	// TODO: check next step position area (10 directions)
	else
	{
		UAV *_otherUav = NULL;
		_uavNum = isUavInArea(_uav.nPath[_uav.nCurrentPathIndex], uavNo);
		if (_uavNum > 1)
		{
			for (int i = 0; i < _uavNum; i++)
			{
				if (uavNo[i] < 1000)
				{
					if (uavNo[i] == _uav.nNO)
						continue;
					_otherUav = &(match->astWeUav[uavNo[i]]);
					if (_otherUav->nAction == UAV_ACTION::UAV_STANDBY)
					{
						continue;
					}

					if (_otherUav->nIsMoved)
					{
						if (_otherUav->nPath[_otherUav->nCurrentPathIndex + 1] == _uav.nPath[_uav.nCurrentPathIndex])
						{
							if (_uav.nPos.z + 1 <= map->nHHigh)
							{
								tmpPoint_4 = _uav.nPos;
								tmpPoint_4.z += 1;
								tmpMark_1 = getMapValue(statusMap, tmpPoint_4);
								if (tmpMark_1 == AREA_OBJ::IS_FOG || tmpMark_1 == AREA_OBJ::IS_NULL)
								{
									uavDodgeAndGetNewPath(_uav, tmpPoint_4);
									return MOVE_ACTION::M_NEWPATH;
								}
								tmpPoint_4.z -= 2;
								if (tmpMark_1 == AREA_OBJ::IS_FOG || tmpMark_1 == AREA_OBJ::IS_NULL)
								{
									uavDodgeAndGetNewPath(_uav, tmpPoint_4);
									return MOVE_ACTION::M_NEWPATH;
								}
								return MOVE_ACTION::M_STANDBY;
							}
						}
						continue;
					}
					else
					{
						if (_otherUav->nPath[_otherUav->nCurrentPathIndex + 1] == _uav.nPath[_uav.nCurrentPathIndex])
						{
							_otherUav->nIsMoved = true;
							continue;
						}
						else
						{
							continue;
						}
					}
				}
			}
			return MOVE_ACTION::M_NORMAL;
		}
		else
		{
			return MOVE_ACTION::M_NORMAL;
		}
	}
}

bool UAVAI::getPath(UAV & _uav)
{
	if(_uav.nPathLength > 0)
		clearUavPath(_uav);
	Point3 _tmpPoint;
	// take off
	if (_uav.nPos.x != _uav.nTarget.x || _uav.nPos.y != _uav.nTarget.y)
	{
		if (_uav.nPos.z < map->nHLow)
		{
			_tmpPoint = _uav.nPos;
			_tmpPoint.z = map->nHLow;
			setUavVirticalPath(_uav.nPos, _tmpPoint, _uav.nPath, _uav.nPathLength);
		}
	}
	// moving
	if (_uav.nPathLength > 0)
	{
		if(!setMinUavHorizontalPath(_uav.nPath[_uav.nPathLength - 1], _uav.nTarget, _uav.nPath, _uav.nPathLength))
		{
			clearUavPath(_uav);
			return false;
		}
	}
	else
	{
		if(!setMinUavHorizontalPath(_uav.nPos, _uav.nTarget, _uav.nPath, _uav.nPathLength))
		{
			clearUavPath(_uav);
			return false;
		}
	}
	// landing
	if (_uav.nPathLength > 0)
	{
		if (_uav.nPath[_uav.nPathLength - 1].z != _uav.nTarget.z)
		{
			setUavVirticalPath(_uav.nPath[_uav.nPathLength - 1], _uav.nTarget, _uav.nPath, _uav.nPathLength);
		}
	}
	else
	{
		if (_uav.nPos.z != _uav.nTarget.z)
		{
			setUavVirticalPath(_uav.nPos, _uav.nTarget, _uav.nPath, _uav.nPathLength);
		}
	}
	_uav.nIsGetPath = true;
	if (_uav.nAction != UAV_ACTION::UAV_ATTACK)
	{
		_uav.nAction = UAV_ACTION::UAV_MOVING;
	}
	return true;
}

bool UAVAI::getPath(const Point3 & _from, const Point3 & _to, vector<Point3>& _path, int &_pathLength)
{
	Point3 _tmpPoint;
	int _lastPathLength = _pathLength;
	// take off
	if (_from.x != _to.x || _from.y != _to.y)
	{
		if (_from.z < map->nHLow)
		{
			_tmpPoint = _from;
			_tmpPoint.z = map->nHLow;
			setUavVirticalPath(_from, _tmpPoint, _path, _pathLength);
		}
	}
	// moving
	if (_pathLength > 0)
	{
		if(!setMinUavHorizontalPath(_path[_pathLength - 1], _to, _path, _pathLength))
		{
			_pathLength = _lastPathLength;
			_path.resize(_lastPathLength);
			return false;
		}
	}
	else
	{
		if (!setMinUavHorizontalPath(_from, _to, _path, _pathLength))
		{
			_pathLength = _lastPathLength;
			_path.resize(_lastPathLength);
			return false;
		}
	}
	// landing
	if (_pathLength > 0)
	{
		if (_path[_pathLength - 1].z != _to.z)
		{
			setUavVirticalPath(_path[_pathLength - 1], _to, _path, _pathLength);
		}
	}
	else
	{
		if (_from.z != _to.z)
		{
			setUavVirticalPath(_from, _to, _path, _pathLength);
		}
	}
	return true;
}

bool UAVAI::setUavVirticalPath(const Point3 & _from, const Point3 & _to, vector<Point3> &_path, int &_pathLength)
{
	Point3 _tmpFrom = _from;
	if (_tmpFrom.x == _to.x && _tmpFrom.y == _to.y)
	{
		if (_tmpFrom.z == _to.z)
		{
			return true;
		}
		int _newPathLength = abs(_to.z - _tmpFrom.z);
		int _lastPathLength = _pathLength;
		_pathLength += _newPathLength;
		_path.resize(_pathLength);
		for (int i = 0; i < _newPathLength; i++)
			_path[_lastPathLength + i] = _tmpFrom;
		int _direction;
		if (_to.z > _tmpFrom.z)
			_direction = 1;
		else
			_direction = -1;
		for (int i = 0; i < _newPathLength; i++)
			_path[_lastPathLength + i].z += _direction*(i + 1);
		// check the end of path point
		if (getMapValue(pathMap, _path[_pathLength - 1]) <= AREA_OBJ::IS_BUILDING)
		{
			_pathLength = _lastPathLength;
			_path.resize(_lastPathLength);
			return false;
		}
		else
		{
			return true;
		}
	}
	else
	{
		cout << "Error in setUavVirticalPath" << endl;
		return false;
	}
}

bool UAVAI::setMinUavHorizontalPath(const Point3 & _from, const Point3 & _to, vector<Point3> &_path, int &_pathLength)
{
	if (_from.x == _to.x && _from.y == _to.y)
	{
		return true;
	}
	int _minPathLength = 999999;
	int _tmpPathLength;
	bool _isGetValidPath = false;
	minPath.resize(0);
	tmpPath.resize(0);
	Point3 _tmpToPoint;
	_tmpToPoint.setPoint(_from);
	// calculate every min path in different z height
	for (int i = map->nHLow; i <= map->nHHigh; i++)
	{
		// if the two points are not in the same hight
		_tmpPathLength = 0;
		tmpPath.resize(0);
		_tmpToPoint.z = i;
		// if the virtival path contains barrier 
		if (!setUavVirticalPath(_from, _tmpToPoint, tmpPath, _tmpPathLength))
		{
			continue;
		}
		// if the path searcher can't get the valid path
		if (!getHorizontalPath(_tmpToPoint, _to, _tmpToPoint.z, tmpPath, _tmpPathLength))
		{
			continue;
		}
		// save min path
		if (_tmpPathLength <= _minPathLength)
		{
			_minPathLength = _tmpPathLength;
			minPath = tmpPath;
			_isGetValidPath = true;
		}
	}
	if (_isGetValidPath)
	{
		_pathLength += _minPathLength;
		for (int i = 0; i < _minPathLength; i++)
		{
			_path.push_back(minPath[i]);
		}
		return true;
	}
	return false;
}

bool UAVAI::getHorizontalPath(const Point3 & _from, const Point3 & _to, const int & _z, vector<Point3>& _path, int &_pathLength)
{
	Point3 _tmpFrom, _tmpTo;
	_tmpFrom = _from;
	_tmpTo = _to;
	_tmpFrom.z = _z;
	_tmpTo.z = _z;
	pathSearcher.setMapAndPoint(&pathMap, _tmpFrom, _tmpTo);
	if (pathSearcher.getPath(_path, _pathLength))
		return true;
	else
		return false;
}

Point3 UAVAI::getHorizontalMoveDirection(const Point3 & _from, const Point3 & _to)
{
	Point3 _direction(0, 0, 0);
	Point3 _diff(_to.x - _from.x, _to.y - _from.y, 0);
	if (_diff.x > 0)
		_direction.x = 1;
	else if (_diff.x < 0)
		_direction.x = -1;
	if (_diff.y > 0)
		_direction.y = 1;
	else if (_diff.y < 0)
		_direction.y = -1;
	return _direction;
}

void UAVAI::clearUavPath(UAV & _uav)
{
	_uav.nPath.resize(0);
	_uav.nPathLength = 0;
	_uav.nCurrentPathIndex = -1;
	_uav.nIsGetPath = false;
	if (_uav.nAction != UAV_ACTION::UAV_ATTACK)
	{
		_uav.nAction = UAV_ACTION::UAV_STANDBY;
	}
	_uav.nGoodsTarget = -1;
}

void UAVAI::buyNewUav()
{
	int _purchaseNum = 0;
	int _goodsNo = -1;
	int _tmpPathLen, _bestPathLen, _bestGoodsNo, _bestGoodsIndex;
	int _buyUavIndex;
	double _maxSocres, _tmpScores;
	bool _isGetValidPath = false;
	GOODS *_goods = NULL;
	if (money < map->astUavPrice[cheapestUavIndex].nValue)
	{
		plan->nPurchaseNum = 0;
		return;
	}
	if (UAVAliveNum <= MAX_ALIVE_UAV_NUM)
	{
		_maxSocres = -1;
		_tmpScores = 0;
		minGoodsPath.resize(0);
		_bestPathLen = 0;
		_bestGoodsNo = -1;
		_bestGoodsIndex = -1;

		for (int i = 0; i < match->nGoodsNum; i++)
		{
			tmpGoodsPath.resize(0);
			_tmpPathLen = 0;
			_goodsNo = match->astGoods[i].nNO;
			_goods = &(match->astGoods[i]);

			if (goodsStatus[_goodsNo].isRejectedByHome)
				continue;
			if (goodsStatus[_goodsNo].nCatchedUavNo != -1)
				continue;
			if (match->astGoods[i].nState != 0)
				continue;
			if (!getPath(map->nParkingPos, match->astGoods[i].nStartPos, tmpGoodsPath, _tmpPathLen))
				continue;
			if (_tmpPathLen > match->astGoods[i].nLeftTime)
			{
				goodsStatus[_goodsNo].isRejectedByHome = true;
				continue;
			}

			_tmpScores = (double(_goods->nValue) / double(_tmpPathLen)) *
				(double(_goods->nValue) / double(_goods->nWeight));

			if (_tmpScores > _maxSocres)
			{
				minGoodsPath.swap(tmpGoodsPath);
				_bestPathLen = _tmpPathLen;
				_isGetValidPath = true;
				_bestGoodsNo = _goodsNo;
				_bestGoodsIndex = i;
			}
		}

		if(_isGetValidPath)
		{
			_buyUavIndex = getBuyNewUavIndex(match->astGoods[_bestGoodsIndex]);
			if (map->astUavPrice[_buyUavIndex].nValue < money)
			{
				strcpy(plan->szPurchaseType[_purchaseNum], map->astUavPrice[_buyUavIndex].szType);
				match->astWeUav[UAVNum + _purchaseNum].nAction = UAV_ACTION::UAV_CATCHING;
				money -= map->astUavPrice[_buyUavIndex].nValue;

				match->astWeUav[UAVNum + _purchaseNum].nTarget = match->astGoods[_bestGoodsIndex].nStartPos;
				match->astWeUav[UAVNum + _purchaseNum].nPath.swap(minGoodsPath);
				match->astWeUav[UAVNum + _purchaseNum].nPathLength = _bestPathLen;
				match->astWeUav[UAVNum + _purchaseNum].nCurrentPathIndex = -1;
				match->astWeUav[UAVNum + _purchaseNum].nIsGetPath = true;
				match->astWeUav[UAVNum + _purchaseNum].nGoodsTarget = _bestGoodsNo;
				goodsStatus[_goodsNo].nCatchedUavNo = UAVNum + _purchaseNum;
				goodsStatus[_goodsNo].nIsRejectUav[UAVNum + _purchaseNum] = true;
				_purchaseNum++;
			}
		}
		// buy attacker
		if (UAVAttackerNum < MAX_ATTACKER_UAV_NUM)
		{
			if (map->astUavPrice[cheapestUavIndex].nValue < money)
			{
				strcpy(plan->szPurchaseType[_purchaseNum], map->astUavPrice[cheapestUavIndex].szType);
				match->astWeUav[UAVNum + _purchaseNum].nAction = UAV_ACTION::UAV_ATTACK;
				money -= map->astUavPrice[cheapestUavIndex].nValue;
				UAVAttackerNum++;
				_purchaseNum++;
			}
		}
	}
	plan->nPurchaseNum = _purchaseNum;
}

void UAVAI::initUavValue()
{
	if (initUavValueNum < match->nUavWeNum)
	{
		for (int i = initUavValueNum; i < match->nUavWeNum; i++)
		{
			match->astWeUav[i].nValue = getUavValue(match->astWeUav[i]);
			initUavValueNum++;
		}
	}
}

int UAVAI::getUavValue(UAV & _uav)
{
	for (int i = 0; i < map->nUavPriceNum; i++)
	{
		if (!strcmp(_uav.szType, map->astUavPrice[i].szType))
			return map->astUavPrice[i].nValue;
	}
	return -1;
}

int UAVAI::getBuyNewUavIndex(GOODS & _goods)
{
	int _minDiff = 999999;
	int _tmpDiff = -1;
	int _minIndex = -1;
	for (int i = 0; i < map->nUavPriceNum; i++)
	{
		_tmpDiff = map->astUavPrice[i].nLoadWeight - _goods.nWeight;
		if (_tmpDiff >= 0 && _tmpDiff < _minDiff)
		{
			_minIndex = i;
		}
	}
	return _minIndex;
}

void UAVAI::searchGoods()
{
	// Scores = (value / distance) * utilization * (value / weight)
	double _maxScores, _tmpScores;
	int _tmpPathLen, _bestGoodsIndex ,_goodsNo;
	UAV *_uav = NULL;
	GOODS *_goods = NULL;
	bool _isGetValidPath;
	// Uav-Oriented
	for (int i = 0; i < match->nUavWeNum; i++)
	{
		_uav = &(match->astWeUav[i]);
		if (_uav->nIsCrash)
			continue;
		if (_uav->nAction == UAV_ACTION::UAV_DELIVERYING)
			continue;
		if (_uav->nAction == UAV_ACTION::UAV_ATTACK)
			continue;
		if (_uav->nAction == UAV_ACTION::UAV_CATCHING)
		{
			_goods = &(match->astGoods[getGoodsIndexByNo(_uav->nGoodsTarget)]);
			_maxScores = (double(_goods->nValue) / double(_uav->nPathLength - _uav->nCurrentPathIndex)) *
			             (double(_goods->nWeight) / double(_uav->nLoadWeight)) *
			          	 (double(_goods->nValue) / double(_goods->nWeight));
		}
		else
		{
			_maxScores = -1.0;
		}
		
		minGoodsPath.resize(0);
		_isGetValidPath = false;
		_bestGoodsIndex = -1;

		for (int j = 0; j < match->nGoodsNum; j++)
		{
			_tmpScores = 0;
			_goods = &(match->astGoods[j]);
			_goodsNo = _goods->nNO;
			tmpGoodsPath.resize(0);
			_tmpPathLen = 0;

			if (goodsStatus[_goodsNo].nCatchedUavNo != -1)
				continue;
			if (goodsStatus[_goodsNo].nIsRejectUav[i])
				continue;
			if (_goods->nWeight > _uav->nLoadWeight)
				continue;
			if (!getPath(_uav->nPos, _goods->nStartPos, tmpGoodsPath, _tmpPathLen))
			{
				goodsStatus[_goodsNo].nIsRejectUav[i] = true;
				continue;
			}
			if (_tmpPathLen > _goods->nLeftTime || _tmpPathLen <= 0)
			{
				goodsStatus[_goodsNo].nIsRejectUav[i] = true;
				continue;
			}
			// get scores
			_tmpScores = (double(_goods->nValue) / double(_tmpPathLen)) *
				         (double(_goods->nWeight) / double(_uav->nLoadWeight)) *
				         (double(_goods->nValue) / double(_goods->nWeight));
			// save (maybe something wrong)
			if (_maxScores < _tmpScores)
			{
				goodsStatus[_goodsNo].nIsRejectUav[i] = true;
				_bestGoodsIndex = j;
				_maxScores = _tmpScores;
				minGoodsPath.swap(tmpGoodsPath);
				_isGetValidPath = true;
			}
		}

		if (!_isGetValidPath)
			continue;

		// set goods target
		_goods = &(match->astGoods[_bestGoodsIndex]);
		_goodsNo = _goods->nNO;
		goodsStatus[_goodsNo].nCatchedUavNo = _uav->nNO;
		goodsStatus[_goodsNo].nIsRejectUav[_uav->nNO] = true;

		_uav->nTarget = _goods->nStartPos;
		_uav->nGoodsTarget = _goodsNo;
		_uav->nAction = UAV_ACTION::UAV_CATCHING;
		_uav->nPathLength = minGoodsPath.size();
		_uav->nPath.swap(minGoodsPath);
		_uav->nCurrentPathIndex = -1;
		_uav->nIsGetPath = true;
	}

}

int UAVAI::getGoodsIndexByNo(int _No)
{
	for (int i = 0; i < match->nGoodsNum; i++)
	{
		if (match->astGoods[i].nNO == _No)
			return i;
	}
	return -1;
}

void UAVAI::setAttackTarget()
{
	int _mostValuedEnemyNo = -1;
	int _mostValued = 0;
	for (int i = 0; i < match->nUavWeNum; i++)
	{
		if (match->astWeUav[i].nIsCrash)
			continue;
		if (match->astWeUav[i].nAction != UAV_ACTION::UAV_ATTACK)
			continue;
		if (match->astWeUav[i].nAttackType == ATTACK_TYPE::AT_NULL)
		{
			for (int j = 0; j < match->nUavEnemyNum; j++)
			{
				if (match->astEnemyUav[j].nPos.z <= 0)
					continue;
				if (_mostValued < getUavValue(match->astEnemyUav[j]))
				{
					_mostValued = getUavValue(match->astEnemyUav[j]);
					_mostValuedEnemyNo = match->astEnemyUav[j].nNO;
				}
			}
			if (_mostValuedEnemyNo >= 0)
			{
				match->astWeUav[i].nAttackType = ATTACK_TYPE::AT_UAV;
				match->astWeUav[i].nAttackTarget = _mostValuedEnemyNo;
			}
		}
	}
}

void UAVAI::updateAttackTarget()
{
	for (int i = 0; i < match->nUavWeNum; i++)
	{
		if (match->astWeUav[i].nIsCrash)
			continue;
		if (match->astWeUav[i].nAction != UAV_ACTION::UAV_ATTACK)
			continue;
		if (match->astWeUav[i].nAttackType == ATTACK_TYPE::AT_UAV)
		{
			for (int j = 0; j < match->nUavEnemyNum; j++)
			{
				if (match->astEnemyUav[j].nNO == match->astWeUav[i].nAttackTarget)
				{
					if (match->astEnemyUav[j].nPos.z != -1)
					{
						match->astWeUav[i].nTarget = match->astEnemyUav[j].nPos;
						getPath(match->astWeUav[i]);
						return;
					}
				}
			}
			match->astWeUav[i].nAttackTarget = -1;
			match->astWeUav[i].nAttackType = ATTACK_TYPE::AT_NULL;
			return;
		}
	}
}
