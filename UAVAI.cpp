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
		for (int j = 0; i < map->nMapY; j++)
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
		_p2 =_p1 + Point3(map->astFog[i].nL - 1, map->astFog[i].nW - 1, 0);
		_p2.z = map->astFog[i].nT;
		fillArea(mapArray, _p1, _p2, AREA_OBJ::IS_FOG);
	}
}

void UAVAI::getNextAction()
{
	for (int i = 0; i < match->nUavWeNum; i++)
	{
		// if crashed, continue
		if (match->astWeUav[i].nStatus == UAV_STATUS::UAV_CRASH)
			continue;
		if (match->astWeUav[i].nPos.z < map->nHLow);
	}
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
	_kb.k = double(_p1.y - _p2.y) / double(_p1.x - _p2.x);
	_kb.b = double(_p2.y) - double(_p2.x*(_p1.y - _p2.y)) / double(_p1.x - _p2.x);
}

int UAVAI::getNextY(int _x, KB & _kb)
{
	return ceil(_kb.k*double(_x) + _kb.b);
}
