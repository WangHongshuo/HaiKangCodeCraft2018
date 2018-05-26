#include "UAVAI.h"

UAVAI::UAVAI()
{
	tmpPath.reserve(600);
	minPath.reserve(600);
	tmpGoodsPath.reserve(600);
	minGoodsPath.reserve(600);
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
	}
	goodsStatus.resize(MAX_GOODS_NUM);
	initUavValue();
}

void UAVAI::initMap()
{
	Point3 _p1, _p2;
	// init mapArray
	mapArray.resize(map->nMapX);
	for (int i = 0; i < map->nMapX; i++)
	{
		mapArray[i].resize(map->nMapY);
		for (int j = 0; j < map->nMapY; j++)
		{
			mapArray[i][j].resize(map->nMapZ);
			mapArray[i][j].assign(map->nMapZ, AREA_OBJ::IS_NULL);
		}
	}
	// init fog in the mapArray
	for (int i = 0; i < map->nFogNum; i++)
	{
		_p1.setPoint(map->astFog[i].nX, map->astFog[i].nY, map->astFog[i].nB);
		_p2 = _p1 + Point3(map->astFog[i].nL - 1, map->astFog[i].nW - 1, 0);
		_p2.z = map->astFog[i].nT;
		fillArea(mapArray, _p1, _p2, AREA_OBJ::IS_FOG);
	}
	// init building in the mapArray
	for (int i = 0; i < map->nBuildingNum; i++)
	{
		_p1.setPoint(map->astBuilding[i].nX, map->astBuilding[i].nY, 0);
		_p2 = _p1 + Point3(map->astBuilding[i].nL - 1, map->astBuilding[i].nW - 1, map->astBuilding[i].nH - 1);
		fillArea(mapArray, _p1, _p2, AREA_OBJ::IS_BUILDING);
	}
	statusMap = mapArray;
}

void UAVAI::setInitUavTarget()
{
	// set init target as the center of map (whatever the target obj is)
	Point3 _center, _tP;
	_center.setPoint(map->nMapX / 2, map->nMapY / 2, map->nHLow);
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
		_tmp++;
	}
}

void UAVAI::getNextAction()
{
	// reset uav.isMoved flag
	resetUavMovedFlag();
	// init uav value
	initUavValue();
	// init uav nLastPos
	if (match->nTime == 1)
	{
		initUavLastPos();
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
	// search goods
	searchGoods();

	// get next moving step and copy to plan
	money = match->nWeValue;
	moveAllUavByAction(UAV_ACTION::UAV_CATCHING, UAVNum);
	moveAllUavByAction(UAV_ACTION::UAV_DELIVERYING, UAVNum);
	moveAllUavByAction(UAV_ACTION::UAV_MOVING, UAVNum);
	moveAllUavByAction(UAV_ACTION::UAV_STANDBY, UAVNum);
	UAVAliveNum = 0;
	for (int i = 0; i < match->nUavWeNum; i++)
	{
		if (match->astWeUav[i].nIsCrash)
			continue;
		copyUav(match->astWeUav[i], plan->astUav[UAVAliveNum]);
		UAVAliveNum++;
	}
	cout << "Available money: " << money << endl;
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
	}
}

void UAVAI::moving(UAV & _uav)
{
	// save uav.nPos
	tmpPoint = _uav.nPos;
	_uav.nIsMoved = true;
	// path index +1
	_uav.nCurrentPathIndex++;
	// check pos
	if (_uav.nCurrentPathIndex < _uav.nPathLength - 1)
	{
		// if there is something in the next position, check it
		if (getMapValue(statusMap, _uav.nPath[_uav.nCurrentPathIndex]) >= 0)
		{
			// take off or landing action
			if (_uav.nPath[_uav.nCurrentPathIndex].z != _uav.nPos.z)
			{
				// standby
				_uav.nCurrentPathIndex--;
				return;
			}
			else if (_uav.nPos.z < map->nHHigh)
			{
				// move to next position
				_uav.nPos.z += 1;
			}
			_uav.nPath.resize(0);
			_uav.nPathLength = 0;
			_uav.nCurrentPathIndex = -1;
			getPath(_uav.nPos, _uav.nTarget, _uav.nPath, _uav.nPathLength);
		}
		else
		{
			// move to next position
			_uav.nPos = _uav.nPath[_uav.nCurrentPathIndex];
		}
		// check goods status
		if (_uav.nAction == UAV_ACTION::UAV_CATCHING)
		{
			for (int i = 0; i < match->nGoodsNum; i++)
			{
				if (match->astGoods[i].nNO == _uav.nGoodsTarget)
				{
					if (match->astGoods[i].nState != 0)
					{
						clearUavPath(_uav);
						return;
					}
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
			_uav.nTarget.z = map->nHLow;
			getPath(_uav);
			_uav.nAction = UAV_ACTION::UAV_MOVING;
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
	if (tmpPoint != _uav.nPos)
	{
		// clear, save, set
		setMapValue(statusMap, _uav.nLastPos, _uav.nLastPosMapMark);
		_uav.nLastPos = _uav.nPos;
		_uav.nLastPosMapMark = getMapValue(statusMap, _uav.nPos);
		setMapValue(statusMap, _uav.nPos, _uav.nNO);
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
		if (match->astWeUav[i].nIsGetPath && match->astWeUav[i].nAction == _action)
			moving(match->astWeUav[i]);
		else
			continue;
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
	_uav.nAction = UAV_ACTION::UAV_MOVING;
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
		if (getMapValue(mapArray, _path[_pathLength - 1]) <= AREA_OBJ::IS_BUILDING)
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
		if (_tmpPathLength < _minPathLength)
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
	pathSearcher.setMapAndPoint(&mapArray, _tmpFrom, _tmpTo);
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
	_uav.nAction = UAV_ACTION::UAV_STANDBY;
	_uav.nGoodsTarget = -1;
}

void UAVAI::buyNewUav()
{
	int _purchaseNum = 0;
	if (UAVAliveNum <= map->nMapX / 2)
	{
		for (int i = 0; i < map->nUavPriceNum; i++)
		{
			if (map->astUavPrice[i].nValue < money)
			{
				strcpy(plan->szPurchaseType[_purchaseNum], map->astUavPrice[i].szType);
				_purchaseNum++;
				money -= map->astUavPrice[i].nValue;
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

void UAVAI::searchGoods()
{
	int _minPathLength, _minUavIndex;
	int _tmpPathLength;
	int _goodsNo;
	minGoodsPath.resize(0);
	tmpGoodsPath.resize(0);
	bool _isGetValidPath = false;
	for (int i = 0; i < match->nGoodsNum; i++)
	{
		_goodsNo = match->astGoods[i].nNO;
		if (match->astGoods[i].nState != 0)
			continue;
		if (goodsStatus[_goodsNo].nCatchedUavNo != -1)
			continue;
		_minPathLength = 9999999;
		_isGetValidPath = false;

		for (int j = 0; j < match->nUavWeNum; j++)
		{
			if (match->astWeUav[j].nIsCrash)
				continue;
			if (match->astWeUav[j].nGoodsTarget != -1)
				continue;
			if (match->astWeUav[j].nAction == UAV_ACTION::UAV_CATCHING)
				continue;
			if (match->astWeUav[j].nAction == UAV_ACTION::UAV_DELIVERYING)
				continue;
			if (match->astWeUav[j].nLoadWeight < match->astGoods[i].nWeight)
				continue;

			_tmpPathLength = 0;
			tmpGoodsPath.resize(0);
			// if can't get path
			if (!getPath(match->astWeUav[j].nPos, match->astGoods[i].nStartPos, tmpGoodsPath, _tmpPathLength))
				continue;
			if (_tmpPathLength == 0)
				continue;
			if (_tmpPathLength < _minPathLength)
			{
				_minPathLength = _tmpPathLength;
				minGoodsPath = tmpGoodsPath;
				_minUavIndex = j;
				_isGetValidPath = true;
			}
		}
		if (match->astGoods[i].nLeftTime <= _minPathLength || !_isGetValidPath)
		{
			continue;
		}
		goodsStatus[_goodsNo].nCatchedUavNo = _minUavIndex;
		clearUavPath(match->astWeUav[_minUavIndex]);
		match->astWeUav[_minUavIndex].nTarget = match->astGoods[i].nStartPos;
		match->astWeUav[_minUavIndex].nGoodsTarget = match->astGoods[i].nNO;
		match->astWeUav[_minUavIndex].nAction = UAV_ACTION::UAV_CATCHING;
		match->astWeUav[_minUavIndex].nPath = minGoodsPath;
		match->astWeUav[_minUavIndex].nIsGetPath = true;
		match->astWeUav[_minUavIndex].nPathLength = _minPathLength;
	}
}

