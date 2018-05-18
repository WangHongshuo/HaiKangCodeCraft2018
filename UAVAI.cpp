#include "UAVAI.h"

UAVAI::UAVAI()
{
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
			mapArray[i][j].assign(map->nMapZ, 0);
		}
	}
	// init building in the mapArray
	for (int i = 0; i < map->nBuildingNum; i++)
	{
		_p1.setPoint(map->astBuilding[i].nX, map->astBuilding[i].nY, 0);
		_p2 = _p1 + Point3(map->astBuilding[i].nL - 1, map->astBuilding[i].nW - 1, map->astBuilding[i].nH);
		fillArea(mapArray, _p1, _p2, AREA_OBJ::IS_BUILDING);
	}
	// init fog in the mapArray
	for (int i = 0; i < map->nFogNum; i++)
	{
		_p1.setPoint(map->astFog[i].nX, map->astFog[i].nY, map->astFog[i].nB);
		_p2 = _p1 + Point3(map->astFog[i].nL - 1, map->astFog[i].nW - 1, 0);
		_p2.z = map->astFog[i].nT;
		fillArea(mapArray, _p1, _p2, AREA_OBJ::IS_FOG);
	}
}

void UAVAI::setInitUavTarget()
{
	// set init target as the center of map (whatever the target obj is)
	Point3 _center, _tP;
	_center.setPoint(map->nMapX / 2, map->nMapY / 2, map->nMapZ);
	match->astWeUav[0].nTarget = _center;

}

void UAVAI::getNextAction()
{
	for (int i = 0; i < match->nUavWeNum; i++)
	{
		// if crashed, continue
		if (match->astWeUav[i].nStatus == UAV_STATUS::UAV_CRASH)
			continue;
		// if uav in the parking land, take off
		if (match->astWeUav[i].nAction == UAV_ACTION::UAV_INPARKING)
		{
			match->astWeUav[i].nAction = UAV_ACTION::UAV_TAKEOFF;
			match->astWeUav[i].nTarget.setPoint(map->nParkingPos + Point3(0, 0, map->nHLow));
			moving(match->astWeUav[i]);
			continue;
		}

		if (match->astWeUav[i].nAction == UAV_ACTION::UAV_TAKEOFF)
		{
			moving(match->astWeUav[i]);
			continue;
		}

		if (match->astWeUav[i].nAction == UAV_ACTION::UAV_STANDBY)
		{
			// if the uav finished take off action

		}

	}
}

int UAVAI::getMapArrayValue(const vector<vector<vector<int>>>& _array, const Point3 & _p)
{
	return _array[_p.x][_p.y][_p.z];
}

bool UAVAI::isPointInTheMap(Point3 & _p)
{
	if (_p.x > map->nMapX - 1 || _p.y > map->nMapY - 1 || _p.z > map->nMapZ - 1 ||
		_p.x < 0 || _p.y < 0 || _p.z < 0)
		return false;
	else
		return true;
}

void UAVAI::fillArea(vector<vector<vector<int>>>& _Array, const Point3 & _p1, const Point3 & _p2, int _fill)
{
	for (int i = _p1.x; i <= _p2.x; i++)
		for (int j = _p1.y; j <= _p2.y; j++)
			for (int k = _p1.z; k <= _p2.z; k++)
				_Array[i][j][k] = _fill;
}

void UAVAI::getKBofLineEqu(KB & _kb, const Point3 & _p1, const Point3 & _p2)
{
	if (_p1.x - _p2.x == 0)
	{
		_kb.isInf = true;
	}
	else
	{
		_kb.k = double(_p1.y - _p2.y) / double(_p1.x - _p2.x);
		_kb.b = double(_p2.y) - double(_p2.x) * _kb.k;
		_kb.isInf = false;
	}
}

int UAVAI::getNextY(const int _x, const KB & _kb)
{
	if (_kb.isInf)
		return 0;
	else
		return ceil(_kb.k*double(_x) + _kb.b);
}

void UAVAI::moving(UAV & _uav)
{
	switch (_uav.nAction)
	{
	case UAV_ACTION::UAV_MOVING:
	{
		break;
	}
	case UAV_ACTION::UAV_TAKEOFF:
	{

		_uav.nPos.z += 1;
		break;
	}
	case UAV_ACTION::UAV_LANDING:
	{
		_uav.nPos.z -= 1;
		break;
	}
	case UAV_ACTION::UAV_INPARKING:
	{
		break;
	}
	default:
	{
		break;
	}
	}

	// check action
	if (_uav.nPos == _uav.nTo)
	{
		_uav.nAction = UAV_ACTION::UAV_STANDBY;
	}
}

UAVAI::AREA_OBJ UAVAI::checkNextStep(const UAV & _uav)
{
	switch (mapArray[_uav.nPos.x][_uav.nPos.y][_uav.nPos.z])
	{
	case AREA_OBJ::IS_NULL:
		return AREA_OBJ::IS_NULL;
	case AREA_OBJ::IS_BUILDING:
		return AREA_OBJ::IS_BUILDING;
	case AREA_OBJ::IS_FOG:
		return AREA_OBJ::IS_FOG;
	default:
		return AREA_OBJ::IS_BUILDING;
	}
}

void UAVAI::getNextToPos(UAV & _uav)
{

}

void UAVAI::getNextStep(UAV & _uav)
{
	getKBofLineEqu(_uav.kb, _uav.nPos, _uav.nTo);
}

void UAVAI::setUavVirticalPath(const Point3 & _from, const Point3 & _to, vector<Point3>& _path)
{
	if (_from.x == _to.x && _from.y == _to.y)
	{
		int _pathLength = abs(_to.z - _from.z);
		_path.resize(_pathLength);
		for (int i = 0; i < _pathLength; i++)
			_path[i] = _from;
		int _direction;
		if (_to.z > _from.z)
			_direction = 1;
		else
			_direction = -1;
		for (int i = 0; i < _pathLength; i++)
			_path[i].z += _direction*(i+1);
	}
	else
	{
		cout << "Error in setUavVirticalPath" << endl;
	}
}

