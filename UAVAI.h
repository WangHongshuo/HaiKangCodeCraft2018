#ifndef __UAVAI_H__
#define __UAVAI_H__

#include "CmdParse.h"
#include "Point3.h"
#include <iostream>
#include <vector>
#include <math.h>

using std::vector;

class UAVAI
{
public:
	UAVAI();
	~UAVAI();
	typedef enum _AREA_OBJ_ { IS_NULL = 0, IS_BUILDING, IS_FOG, IS_ALLY, IS_ENEMY } AREA_OBJ;
	void initPtr(MAP_INFO * _map, MATCH_STATUS * _match, FLAY_PLANE * _flayPlane);
	void initMap();
	void getNextAction();
private:
	bool isInitPtr = false;
	MAP_INFO *map = NULL;
	MATCH_STATUS *match = NULL;
	FLAY_PLANE *plan = NULL;
	vector<vector<vector<int>>> mapArray;
	void fillArea(vector<vector<vector<int>>> &_Array, const Point3 &_p1, const Point3 &_p2, int _fill);
	// y=kx+b
	void getKBofLineEqu(KB &_kb, const Point3 &_p1, const Point3 &_p2);
	int getNextY(int _x, KB &_kb);
	// move action
	//void moveTo(const Point3 &_from, const Point3 &_to, const vector<vector<vector<int>>> &_mapArray, UAV &_uav);

};

#endif // !__UAVAI_H__
