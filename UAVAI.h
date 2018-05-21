#ifndef __UAVAI_H__
#define __UAVAI_H__

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <math.h>
#include "CmdParse.h"
#include "Point3.h"
#include "AStar.h"

using std::vector;
using std::cout;
using std::endl;

class UAVAI
{
public:
	UAVAI();
	~UAVAI();
	typedef enum _AREA_OBJ_ { IS_OUTSIDE = -4, IS_BUILDING, IS_FOG, IS_ENEMY, IS_NULL } AREA_OBJ;
	void initPtr(MAP_INFO * _map, MATCH_STATUS * _match, FLAY_PLANE * _flayPlane);
	void initMap();
	void setInitUavTarget();
	void getNextAction();
private:
	bool isInitPtr = false;
	MAP_INFO *map = NULL;
	MATCH_STATUS *match = NULL;
	FLAY_PLANE *plan = NULL;
	vector<Point3> tmpPath;
	vector<vector<vector<int>>> mapArray;
	AStar pathSearcher;
	int getMapArrayValue(const vector<vector<vector<int>>> &_array, const Point3 &_p);
	bool isPointInTheMap(Point3 &_p);
	void fillArea(vector<vector<vector<int>>> &_Array, const Point3 &_p1, const Point3 &_p2, int _fill);
	// y=kx+b
	void getKBofLineEqu(KB &_kb, const Point3 &_p1, const Point3 &_p2);
	int getNextY(const int _x, const KB &_kb);
	// move action
	void moving(UAV &_uav);
	AREA_OBJ checkNextStep(const Point3 &_p);
	// get uav.nTo
	void getNextToPos(UAV &_uav);
	// get next step direction
	void getNextStep(UAV &_uav);
	// get uav path vector<Point3>
	void getPath(UAV &_uav);
	void setUavVirticalPath(const Point3 & _from, const Point3 & _to, vector<Point3> &_path, int &_pathLength);
	void setMinUavHorizontalPath(const Point3 &_from, const Point3 &_to, UAV &_uav);
	bool getHorizontalPath(const Point3 &_from, const Point3 &_to, const int &_z, vector<Point3> &_path, int &_pathLength);
	Point3 getHorizontalMoveDirection(const Point3 &_from, const Point3 &_to);
	// clear uav path
	void clearUavPath(UAV &_uav);
};

#endif // !__UAVAI_H__
