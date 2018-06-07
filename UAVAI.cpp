#include "UAVAI.h"

UAVAI::UAVAI()
{
	tmpPath.reserve(600);
	minPath.reserve(600);
	tmpGoodsPath.reserve(600);
	minGoodsPath.reserve(600);
	availablePoints.reserve(11);
	tmpUavScope_1.reserve(11);
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

void UAVAI::initPtr(MAP_INFO *_map, MATCH_STATUS *_match, FLY_PLANE *_flayPlane)
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
		match->astEnemyUav[i].nIsEnemy = true;
	}
	goodsStatus.resize(MAX_GOODS_NUM);
	for (int i = 0; i < MAX_GOODS_NUM; i++)
	{
		goodsStatus[i].nIsRejectUav.resize(MAX_UAV_NUM);
		goodsStatus[i].nPath.reserve(600);
	}
	uavNo.reserve(10);
	initUavInfo();
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
		if (map->astBuilding[i].nH > MAX_PATH_HEIGHT)
			MAX_PATH_HEIGHT = map->astBuilding[i].nH;
		_p1.setPoint(map->astBuilding[i].nX, map->astBuilding[i].nY, 0);
		_p2 = _p1 + Point3(map->astBuilding[i].nL - 1, map->astBuilding[i].nW - 1, map->astBuilding[i].nH - 1);
		fillArea(pathMap, _p1, _p2, AREA_OBJ::IS_BUILDING);
	}
	statusMap = pathMap;
	restoredMap = pathMap;
	if (MAX_PATH_HEIGHT < map->nHLow)
		MAX_PATH_HEIGHT = map->nHLow;
	MAX_PATH_HEIGHT += 3;
	if (MAX_PATH_HEIGHT > map->nHHigh)
		MAX_PATH_HEIGHT = map->nHHigh;
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
	for (int i = match->nUavWeNum - 1; i >= 0; i--)
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
	initUavInfo();
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

	// update goods deliverying path
	updateGoodsDeliveryingPath();

	// search goods
	searchGoods();

	updateChargingFlag();

	// get next moving step and copy to plan
	money = match->nWeValue;
	moveAllUavByAction(UAV_ACTION::UAV_DELIVERYING, UAVNum);
	moveAllUavByAction(UAV_ACTION::UAV_CATCHING, UAVNum);
	moveAllUavByAction(UAV_ACTION::UAV_MOVING, UAVNum);
	moveAllUavByAction(UAV_ACTION::UAV_ATTACK, UAVNum);
	moveAllUavByAction(UAV_ACTION::UAV_GO_CHARGING, UAVNum);
	moveAllUavByAction(UAV_ACTION::UAV_STANDBY, UAVNum);

	updateChargingFlag();
	updatePowerInfo();

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
	_dst.nRemainPower = _src.nRemainPower;
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
	Point3 _tmpPoint_1;
	_uav.nIsMoved = true;
	// if uav is charging
	if (_uav.nIsUavCharging && !_uav.nIsFullPower && _uav.nAction != UAV_ACTION::UAV_ATTACK)
	{
		updateWeUavMark(_uav);
		return;
	}
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
					_tmpPoint_1 = match->astGoods[i].nStartPos;
					_tmpPoint_1.z = map->nHLow - 1;
					if (match->astGoods[i].nState != 0 ||
						(getMapValue(statusMap, _tmpPoint_1) >= 1000 && _uav.nPos.z >= map->nHLow) ||
						match->astGoods[i].nLeftTime < _uav.nPathLength - _uav.nCurrentPathIndex)
					{
						goodsStatus[match->astGoods[i].nNO].nIsRejectUav[_uav.nNO] = true;
						_tmpPoint_1.z = map->nHLow + 2;
						clearUavPath(_uav);
						_uav.nTarget = _tmpPoint_1;
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
			int _goodsNo;
			for (int i = 0; i < match->nGoodsNum; i++)
			{
				_goodsNo = match->astGoods[i].nNO;
				if (_goodsNo == _uav.nGoodsTarget)
				{
					match->astGoods[i].nState = 1;
					_uav.nCurrentPathIndex = -1;
					if (goodsStatus[_goodsNo].nPathLength > 0)
					{
						_uav.nPath.swap(goodsStatus[_goodsNo].nPath);
						_uav.nPathLength = goodsStatus[_goodsNo].nPathLength;
					}
					else
					{
						clearUavPath(_uav);
						_uav.nTarget = match->astGoods[i].nEndPos;
						getPath(_uav);
					}
					_uav.nGoodsNo = _uav.nGoodsTarget;
					_uav.nLoadGoodsWeight = match->astGoods[i].nWeight;
					_uav.nAction = UAV_ACTION::UAV_DELIVERYING;
					break;
				}
			}
		}
		else if (_uav.nAction == UAV_ACTION::UAV_DELIVERYING)
		{
			_uav.nRemainPower -= _uav.nLoadGoodsWeight;
			clearUavPath(_uav);
			_uav.nTarget.setPoint(_uav.nPos);
			_uav.nTarget.z = MAX_PATH_HEIGHT;
			getPath(_uav);
			_uav.nAction = UAV_ACTION::UAV_MOVING;
			for (int i = 0; i < MAX_GOODS_NUM; i++)
			{
				goodsStatus[i].nIsRejectUav[_uav.nNO] = false;
			}
		}
		else if (_uav.nAction == UAV_ACTION::UAV_GO_CHARGING)
		{
			_uav.nIsUavCharging = true;
			_uav.nIsFullPower = false;
			clearUavPath(_uav);
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
	if (moveAction != MOVE_ACTION::M_NEWPATH)
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
		_uav.nMovedFeature = _uav.nPos + _uav.nLastPos;
		_uav.nIsPositionChanged = true;
		tmpMark_2 = getMapValue(statusMap, _uav.nLastPos);
		if (tmpMark_2 == _uav.nNO)
		{
			tmpMark_2 = getMapValue(restoredMap, _uav.nLastPos);
			setMapValue(statusMap, _uav.nLastPos, tmpMark_2);
		}
		setMapValue(statusMap, _uav.nPos, _uav.nNO);
		_uav.nLastPos = _uav.nPos;
	}
	else
	{
		_uav.nMovedFeature.setPoint(0, 0, 0);
		_uav.nIsPositionChanged = false;
	}
	setMapValue(statusMap, map->nParkingPos, -1);
}

int UAVAI::getMoveDirection(UAV & _uav)
{
	if (_uav.nPos.z == _uav.nPath[_uav.nCurrentPathIndex].z)
		return MOVE_DIRECTION::M_HORIZONTAL;
	else
	{
		if (_uav.nPos.z < _uav.nPath[_uav.nCurrentPathIndex].z)
			return MOVE_DIRECTION::M_UPWARD;
		else
			return MOVE_DIRECTION::M_DOWNWARD;
	}
}

bool UAVAI::isPathCross(const UAV & _dodgeUav, const Point3 & _uavPos, const Point3 & _uavNextPos)
{
	if (_uavPos + _uavNextPos == _dodgeUav.nMovedFeature)
		return true;
	else
		return false;
}

template<typename T>
int UAVAI::isUavInArea(const Point3 & _p, vector<int>& _uavNo, T &_area)
{
	_uavNo.resize(0);
	_area.resize(0);
	int _No = -1;
	int _num = 0;
	getUavMoveScope(_p, _area);
	typename T::iterator _it;
	for (_it = _area.begin(); _it != _area.end(); _it++)
	{
		_No = getMapValue(statusMap, (*_it));
		if (_No >= 0)
		{
			_uavNo.push_back(_No);
			_num++;
		}
	}
	return _num;
}

bool UAVAI::isUavInArea(const Point3 &_p, int & _ignoredNo, CHECK_OPT _opt)
{
	tmpUavScope_1.resize(0);
	int _tmpMark = -1;
	getUavMoveScope(_p, tmpUavScope_1);
	for (int i = 0; i<int(tmpUavScope_1.size()); i++)
	{
		_tmpMark = getMapValue(statusMap, tmpUavScope_1[i]);
		if (_tmpMark >= 0)
		{
			if (_tmpMark == _ignoredNo)
				continue;
			if (_opt == CHECK_OPT::CO_ALL)
				return true;
			if (_opt == CHECK_OPT::CO_ENEMY && _tmpMark >= 1000)
				return true;
			if (_opt == CHECK_OPT::CO_ALLY && _tmpMark < 1000)
				return true;
		}
	}
	return false;
}

int UAVAI::fixDodgeArea(list<Point3>& _area, UAV & _dodgeUav, UAV &_uav)
{
	if (_dodgeUav.nIsEnemy)
	{
		for (it_1 = _area.begin(); it_1 != _area.end();)
		{
			if (getDistanceInScope(_dodgeUav.nPos, (*it_1)) <= 2)
				it_1 = _area.erase(it_1);
			else
				++it_1;
		}
		return 1;
	}
	else
	{
		if (_dodgeUav.nAction == UAV_ACTION::UAV_STANDBY)
		{
			fixDodgeArea(_area, _dodgeUav.nPos);
			return 0;
		}
		int _dCurIdx = _dodgeUav.nCurrentPathIndex;
		int _cCurIdx = _uav.nCurrentPathIndex;
		if (_dodgeUav.nIsPositionChanged )
		{
			fixDodgeArea(_area, _dodgeUav.nPos);
			if (_dCurIdx >= 0)
			{
				if (_dodgeUav.nPath[_dCurIdx] == _uav.nPos)
				{
					if (_dCurIdx - 1 >= 0)
					{
						fixDodgeArea(_area, _dodgeUav.nPath[_dCurIdx - 1]);
					}
					else
					{
						fixDodgeArea(_area, _dodgeUav.nFrom);
					}
					return 1;
				}
			}
			if (_cCurIdx >= 0)
			{
				if (isPathCross(_dodgeUav, _uav.nPos, _uav.nPath[_cCurIdx]))
					return 1;
				else
					return 0;
			}
			else
			{
				return 0;
			}
		}
		else
		{
			fixDodgeArea(_area, _dodgeUav.nPos);
			if (_dCurIdx + 1 <= _dodgeUav.nPathLength - 1)
			{
				if (_dodgeUav.nPath[_dCurIdx + 1] == _uav.nPos)
				{
					return 1;
				}
			}
		}
		return 0;
	}
}

void UAVAI::fixDodgeArea(list<Point3>& _area, const Point3 & _removedPoint)
{
	for (it_1 = _area.begin(); it_1 != _area.end();)
	{
		if (*(it_1) == _removedPoint)
		{
			it_1 = _area.erase(it_1);
			return;
		}
		else
			++it_1;
	}
}

void UAVAI::doubleCheckDodgeArea(list<Point3>& _area, int &_ignoredNo, CHECK_OPT _opt)
{
	for (it_2 = _area.begin(); it_2 != _area.end();)
	{
		if (isUavInArea(*(it_2), _ignoredNo, _opt))
			it_2 = _area.erase(it_2);
		else
			++it_2;
	}
}

Point3 UAVAI::getBestDodgePositon(list<Point3>& _area, UAV & _uav)
{
	Point3 _tmpPoint;
	int _minDistance = 999999;
	int _tmpDistance = -1;
	for (it_3 = _area.begin(); it_3 != _area.end(); it_3++)
	{
		_tmpDistance = getHorizontalDistance(_uav.nTarget, *it_3);
		if (_minDistance > _tmpDistance)
		{
			_minDistance = _tmpDistance;
			_tmpPoint = *it_3;
		}
	}
	return _tmpPoint;
}

bool UAVAI::isPositionInMap(const Point3 & _p)
{
	if (_p.x < 0 || _p.x > map->nMapX - 1)
		return false;
	if (_p.y < 0 || _p.y > map->nMapY - 1)
		return false;
	if (_p.z < 0 || _p.z > map->nMapZ - 1)
		return false;
	return true;
}

int UAVAI::getDistanceInScope(const Point3 & _p1, const Point3 & _p2)
{
	// the weight of height is set to 2
	return int(pow(_p1.x - _p2.x, 2)) + int(pow(_p1.y - _p2.y, 2)) + int(abs(_p1.z - _p2.z) * 2);
}

int UAVAI::getHorizontalDistance(const Point3 & _p1, const Point3 & _p2)
{
	return int(pow(_p1.x - _p2.x, 2)) + int(pow(_p1.y - _p2.y, 2));
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
	_uav.nFrom = _uav.nPos;
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

UAV * UAVAI::getUavPtrByNo(int _No)
{
	if (_No >= 1000)
	{
		_No %= 1000;
		for (int i = 0; i < match->nUavEnemyNum; i++)
			if (_No == match->astEnemyUav[i].nNO)
				return &(match->astEnemyUav[i]);
	}
	else
	{
		return &(match->astWeUav[_No]);
	}
	return nullptr;
}

// @ only check the position is null or not
template<typename T>
void UAVAI::getUavMoveScope(const Point3 & _center, T& _scope)
{
	if (!_scope.empty())
		_scope.resize(0);
	int _tmpMark;
	int _startIndex;
	int _endIndex = MOVE_DIRECTION_NUM;
	if (_center.z < map->nHLow)
	{
		_startIndex = 8;
	}
	else if (_center.z <= map->nHHigh - 1)
	{
		_startIndex = 0;
	}
	else
	{
		_startIndex = 0;
		_endIndex -= 1;
	}
	for (int i = _startIndex; i < _endIndex; i++)
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
	int _isDodge = 0;
	Point3 _tmpPoint;
	// uav without valid path
	if (_uav.nAction == UAV_ACTION::UAV_STANDBY ||
		(_uav.nAction == UAV_ACTION::UAV_ATTACK && !_uav.nIsGetPath))
	{
		_uavNum = isUavInArea(_uav.nPos, uavNo, dodgePosition);
		if (_uavNum > 0)
		{
			for (int i = 0; i < _uavNum; i++)
			{
				// if there is enemy uav exist
				if (uavNo[i] >= 1000)
				{
					pEnemyUav = getUavPtrByNo(uavNo[i]);
					_isDodge += fixDodgeArea(dodgePosition, *pEnemyUav, _uav);
				}
				else
				{
					pAllyUav = getUavPtrByNo(uavNo[i]);
					if (pAllyUav->nAction == UAV_ACTION::UAV_STANDBY)
						fixDodgeArea(dodgePosition, pAllyUav->nPos);
					else
						_isDodge += fixDodgeArea(dodgePosition, *pAllyUav, _uav);
				}
			}
			// move
			if (_isDodge > 0)
			{
				doubleCheckDodgeArea(dodgePosition, _uav.nNO, CHECK_OPT::CO_ENEMY);
				if (!dodgePosition.empty())
				{
					_uav.nPos = *(dodgePosition.begin());
					_uav.nIsMoved = true;
				}
			}
		}
		updateWeUavMark(_uav);
		return -1;
	}
	// next step map mark
	tmpMark_1 = getMapValue(statusMap, _uav.nPath[_uav.nCurrentPathIndex]);
	uavMoveDirection = getMoveDirection(_uav);
	// if is enemy
	if (tmpMark_1 >= 1000)
	{
		_uavNum = isUavInArea(_uav.nPos, uavNo, dodgePosition);
		for (int i = 0; i < _uavNum; i++)
		{
			if (uavNo[i] == _uav.nNO)
				continue;
			// if there is enemy uav exist
			if (uavNo[i] >= 1000)
			{
				pEnemyUav = getUavPtrByNo(uavNo[i]);
				_isDodge += fixDodgeArea(dodgePosition, *pEnemyUav, _uav);
			}
			else
			{
				pAllyUav = getUavPtrByNo(uavNo[i]);
				if (pAllyUav->nAction == UAV_ACTION::UAV_STANDBY)
					fixDodgeArea(dodgePosition, pAllyUav->nPos);
				else
					_isDodge += fixDodgeArea(dodgePosition, *pAllyUav, _uav);
			}
		}
		doubleCheckDodgeArea(dodgePosition, _uav.nNO, CHECK_OPT::CO_ENEMY);
		if (!dodgePosition.empty())
		{
			_tmpPoint = getBestDodgePositon(dodgePosition, _uav);
			uavDodgeAndGetNewPath(_uav, _tmpPoint);
			return MOVE_ACTION::M_NEWPATH;
		}
		else
		{
			return MOVE_ACTION::M_STANDBY;
		}
	}
	// if is ally
	else if (tmpMark_1 >= 0)
	{
		pAllyUav = &(match->astWeUav[tmpMark_1]);

		if (pAllyUav->nAction == UAV_ACTION::UAV_STANDBY)
		{
			if (!pAllyUav->nIsMoved)
			{
				environmentAware(*pAllyUav);
				return MOVE_ACTION::M_NORMAL;
			}
			else
			{
				return MOVE_ACTION::M_STANDBY;
			}
		}
		if (pAllyUav->nAction == UAV_ACTION::UAV_ATTACK && !pAllyUav->nIsGetPath)
		{
			environmentAware(*pAllyUav);
			return MOVE_ACTION::M_NORMAL;
		}
		if (!pAllyUav->nIsMoved)
		{
			moving(*pAllyUav);
			return MOVE_ACTION::M_NORMAL;
		}
		if (!pAllyUav->nIsMoved && pAllyUav->nPath[pAllyUav->nCurrentPathIndex + 1] != _uav.nPos)
		{
			return MOVE_ACTION::M_NORMAL;
		}

		_uavNum = isUavInArea(_uav.nPos, uavNo, dodgePosition);
		for (int i = 0; i < _uavNum; i++)
		{
			if (uavNo[i] == _uav.nNO)
				continue;
			// if there is enemy uav exist
			if (uavNo[i] >= 1000)
			{
				pEnemyUav = getUavPtrByNo(uavNo[i]);
				_isDodge += fixDodgeArea(dodgePosition, *pEnemyUav, _uav);
			}
			else
			{
				pAllyUav = getUavPtrByNo(uavNo[i]);
				if (pAllyUav->nAction == UAV_ACTION::UAV_STANDBY)
					fixDodgeArea(dodgePosition, pAllyUav->nPos);
				else
					_isDodge += fixDodgeArea(dodgePosition, *pAllyUav, _uav);
			}
		}
		doubleCheckDodgeArea(dodgePosition, _uav.nNO, CHECK_OPT::CO_ENEMY);
		if (!dodgePosition.empty())
		{
			_tmpPoint = getBestDodgePositon(dodgePosition, _uav);
			uavDodgeAndGetNewPath(_uav, _tmpPoint);
			return MOVE_ACTION::M_NEWPATH;
		}
		else
		{
			return MOVE_ACTION::M_STANDBY;
		}
	}
	// check next step position area (10 directions)
	else
	{
		bool _isNextStepGood = true;
		_uavNum = isUavInArea(_uav.nPath[_uav.nCurrentPathIndex], uavNo, dodgePosition);
		if (_uavNum > 1)
		{
			for (int i = 0; i < _uavNum; i++)
			{
				if (uavNo[i] == _uav.nNO)
					continue;
				// if there is enemy uav exist
				if (uavNo[i] >= 1000)
				{
					_isNextStepGood = false;
					break;
				}
				else
				{
					pAllyUav = getUavPtrByNo(uavNo[i]);
					if (pAllyUav->nAction == UAV_ACTION::UAV_STANDBY)
						fixDodgeArea(dodgePosition, pAllyUav->nPos);
					else
						_isDodge += fixDodgeArea(dodgePosition, *pAllyUav, _uav);
				}
				if (_isDodge > 0)
				{
					_isNextStepGood = false;
					break;
				}
			}
			if (!_isNextStepGood)
			{
				_uavNum = isUavInArea(_uav.nPos, uavNo, dodgePosition);
				for (int i = 0; i < _uavNum; i++)
				{
					fixDodgeArea(dodgePosition, _uav.nPath[_uav.nCurrentPathIndex]);
					// if there is enemy uav exist
					if (uavNo[i] >= 1000)
					{
						pEnemyUav = getUavPtrByNo(uavNo[i]);
						_isDodge += fixDodgeArea(dodgePosition, *pEnemyUav, _uav);
					}
					else
					{
						pAllyUav = getUavPtrByNo(uavNo[i]);
						if (pAllyUav->nAction == UAV_ACTION::UAV_STANDBY)
							fixDodgeArea(dodgePosition, pAllyUav->nPos);
						else
							_isDodge += fixDodgeArea(dodgePosition, *pAllyUav, _uav);
					}
				}
				doubleCheckDodgeArea(dodgePosition, _uav.nNO, CHECK_OPT::CO_ENEMY);
				if (!dodgePosition.empty())
				{
					_tmpPoint = getBestDodgePositon(dodgePosition, _uav);
					uavDodgeAndGetNewPath(_uav, _tmpPoint);
					return MOVE_ACTION::M_NEWPATH;
				}
				else
				{
					return MOVE_ACTION::M_STANDBY;
				}
			}
			else
			{
				return MOVE_ACTION::M_NORMAL;
			}
		}
		else
		{
			return MOVE_ACTION::M_NORMAL;
		}
	}
}

bool UAVAI::getPath(UAV & _uav)
{
	if (_uav.nPathLength > 0)
		clearUavPath(_uav);
	_uav.nFrom = _uav.nPos;
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
		if (!setMinUavHorizontalPath(_uav.nPath[_uav.nPathLength - 1], _uav.nTarget, _uav.nPath, _uav.nPathLength))
		{
			clearUavPath(_uav);
			return false;
		}
	}
	else
	{
		if (!setMinUavHorizontalPath(_uav.nPos, _uav.nTarget, _uav.nPath, _uav.nPathLength))
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
		if (!setMinUavHorizontalPath(_path[_pathLength - 1], _to, _path, _pathLength))
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
	int _newPathLength = abs(_to.z - _tmpFrom.z);
	int _lastPathLength = _pathLength;
	if (_tmpFrom.x == _to.x && _tmpFrom.y == _to.y)
	{
		if (_tmpFrom.z == _to.z)
		{
			return true;
		}
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
			_path[_lastPathLength + i].z += _direction * (i + 1);
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
	for (int i = map->nHLow; i <= MAX_PATH_HEIGHT; i++)
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
	_uav.nLoadGoodsWeight = 0;
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

		if (_isGetValidPath)
		{
			_buyUavIndex = getBuyNewUavIndex(match->astGoods[_bestGoodsIndex]);
			if (map->astUavPrice[_buyUavIndex].nValue < money)
			{
				strcpy(plan->szPurchaseType[_purchaseNum], map->astUavPrice[_buyUavIndex].szType);
				match->astWeUav[UAVNum + _purchaseNum].nAction = UAV_ACTION::UAV_CATCHING;
				money -= map->astUavPrice[_buyUavIndex].nValue;

				match->astWeUav[UAVNum + _purchaseNum].nFrom = map->nParkingPos;
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

void UAVAI::initUavInfo()
{
	if (initUavValueNum < match->nUavWeNum)
	{
		for (int i = initUavValueNum; i < match->nUavWeNum; i++)
		{
			getUavInfo(match->astWeUav[i]);
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

void UAVAI::getUavInfo(UAV & _uav)
{
	for (int i = 0; i < map->nUavPriceNum; i++)
	{
		if (!strcmp(_uav.szType, map->astUavPrice[i].szType))
		{
			_uav.nValue = map->astUavPrice[i].nValue;
			_uav.nCapacity = map->astUavPrice[i].nCapacity;
			_uav.nCharge = map->astUavPrice[i].nCharge;
			return;
		}
	}
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
	int _tmpPathLen, _bestGoodsIndex, _goodsNo;
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
		if (_uav->nAction == UAV_ACTION::UAV_GO_CHARGING)
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

			if (goodsStatus[_goodsNo].nCostPower > _uav->nRemainPower)
			{
				if (j == match->nGoodsNum - 1 && !_isGetValidPath)
				{
					if (_uav->nRemainPower == _uav->nCapacity)
						continue;
					clearUavPath(*_uav);
					_uav->nTarget = map->nParkingPos;
					getPath(*_uav);
					_uav->nAction = UAV_ACTION::UAV_GO_CHARGING;
				}
				continue;
			}
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
		_uav->nFrom = _uav->nPos;
		_uav->nCurrentPathIndex = -1;
		_uav->nIsGetPath = true;
	}
}

void UAVAI::updateGoodsDeliveryingPath()
{
	int _goodsNo;
	GOODS *_goods = NULL;
	for (int i = 0; i < match->nGoodsNum; i++)
	{
		_goodsNo = match->astGoods[i].nNO;
		_goods = &(match->astGoods[i]);
		if (goodsStatus[_goodsNo].nPathLength <= 0)
		{
			getPath(_goods->nStartPos, _goods->nEndPos, goodsStatus[_goodsNo].nPath, goodsStatus[_goodsNo].nPathLength);
			goodsStatus[_goodsNo].nCostPower = _goods->nWeight*(goodsStatus[_goodsNo].nPathLength + 1);
		}
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

void UAVAI::updatePowerInfo()
{
	for (int i = 0; i < match->nUavWeNum; i++)
	{
		if (match->astWeUav[i].nIsCrash)
			continue;
		else if (match->astWeUav[i].nIsUavCharging)
			chargeUav(match->astWeUav[i]);
		else if (match->astWeUav[i].nAction == UAV_ACTION::UAV_DELIVERYING)
			dischargeUav(match->astWeUav[i]);
		else
			continue;
	}
}

void UAVAI::updateChargingFlag()
{
	for (int i = 0; i < match->nUavWeNum; i++)
	{
		if (match->astWeUav[i].nIsCrash)
			continue;
		else if (match->astWeUav[i].nPos == map->nParkingPos)
			match->astWeUav[i].nIsUavCharging = true;
		else
			match->astWeUav[i].nIsUavCharging = false;
	}
}

void UAVAI::chargeUav(UAV & _uav)
{
	_uav.nIsFullPower = false;
	_uav.nRemainPower += _uav.nCharge;
	if (_uav.nRemainPower >= _uav.nCapacity)
	{
		_uav.nRemainPower = _uav.nCapacity;
		_uav.nIsFullPower = true;
	}
}

void UAVAI::dischargeUav(UAV & _uav)
{
	_uav.nRemainPower -= _uav.nLoadGoodsWeight;
	if (_uav.nRemainPower < 0)
		_uav.nRemainPower = 0;
}
