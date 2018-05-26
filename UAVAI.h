#ifndef __UAVAI_H__
#define __UAVAI_H__

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <math.h>
#include <string.h>
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
	enum AREA_OBJ { IS_OUTSIDE = -5, IS_BUILDING, IS_FOG, IS_ENEMY, IS_NULL };
	void initPtr(MAP_INFO * _map, MATCH_STATUS * _match, FLAY_PLANE * _flayPlane);
	void initMap();
	void setInitUavTarget();
	void getNextAction();
private:
	int money = 0;
	int UAVAliveNum = -1;
	int UAVNum = -1;
	int initUavValueNum = 0;
	bool isInitPtr = false;
	MAP_INFO *map = NULL;
	MATCH_STATUS *match = NULL;
	FLAY_PLANE *plan = NULL;
	Point3 tmpPoint;
	vector<Point3> tmpPath;
	vector<Point3> minPath;
	vector<Point3> tmpGoodsPath;
	vector<Point3> minGoodsPath;
	vector<vector<vector<int>>> mapArray;
	vector<vector<vector<int>>> statusMap;
	vector<GOODSSTATUS> goodsStatus;
	AStar pathSearcher;
	// init uav.nLastPos
	void initUavLastPos();
	// copy Uav
	void copyUav(const UAV &_src, UAV &_dst);
	int getMapValue(const vector<vector<vector<int>>> &_array, const Point3 &_p);
	void setMapValue(vector<vector<vector<int>>> &_array, const Point3 &_p, int _v);
	void fillArea(vector<vector<vector<int>>> &_Array, const Point3 &_p1, const Point3 &_p2, int _fill);
	// move action
	void resetUavMovedFlag();
	void moving(UAV &_uav);
	void moveAllUavByAction(UAV_ACTION _action, int &_uavNum);
	// environment-aware
	void environmentAware(UAV &_uav);
	// get uav path vector<Point3>
	bool getPath(UAV &_uav);
	bool getPath(const Point3 &_from, const Point3 &_to, vector<Point3> &_path, int &_pathLength);
	bool setUavVirticalPath(const Point3 & _from, const Point3 & _to, vector<Point3> &_path, int &_pathLength);
	bool setMinUavHorizontalPath(const Point3 & _from, const Point3 & _to, vector<Point3> &_path, int &_pathLength);
	bool getHorizontalPath(const Point3 &_from, const Point3 &_to, const int &_z, vector<Point3> &_path, int &_pathLength);
	Point3 getHorizontalMoveDirection(const Point3 &_from, const Point3 &_to);
	// clear uav path
	void clearUavPath(UAV &_uav);
	// buy new uav
	void buyNewUav();
	void initUavValue();
	int getUavValue(UAV &_uav);
	// search goods
	void searchGoods();
};

#endif // !__UAVAI_H__
