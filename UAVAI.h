#ifndef __UAVAI_H__
#define __UAVAI_H__

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <list>
#include <math.h>
#include <string.h>
#include <time.h>
#include "CmdParse.h"
#include "Point3.h"
#include "AStar.h"

using std::vector;
using std::list;
using std::cout;
using std::endl;
using std::pow;
using std::abs;

class UAVAI
{
public:
	UAVAI();
	~UAVAI();
	enum AREA_OBJ { IS_OUTSIDE = -5, IS_BUILDING, IS_FOG, IS_ENEMY, IS_NULL };
	enum MOVE_ACTION { M_STANDBY, M_NEWPATH, M_NORMAL, M_MOVE_ALLY };
	enum CHECK_OPT { CO_ALL, CO_ALLY, CO_ENEMY };
	void initPtr(MAP_INFO * _map, MATCH_STATUS * _match, FLY_PLANE * _flayPlane);
	void initMap();
	void setInitUavTarget();
	void getNextAction();
private:
	static const int MOVE_DIRECTION_NUM = 10;
	int CHEAPEST_UAV_INDEX = 0;
	int MOST_EXPENSIVE_UAV_INDEX = 0;
	int MAX_PATH_HEIGHT = -1;
	int MAX_ALIVE_UAV_NUM;
	int MAX_ATTACKER_UAV_NUM;
	int money = 0;
	int UAVValues = 0;
	int UAVAliveNum = 0;
	int UAVAttackerNum = 0;
	int UAVNum = -1;
	int initUavValueNum = 0;
	bool isInitPtr = false;
	MAP_INFO *map = NULL;
	MATCH_STATUS *match = NULL;
	FLY_PLANE *plan = NULL;
	Point3 tmpPoint_1; // @ in getUavMoveScope();
	Point3 enemyParkingPos;
	vector<int> uavNo;
	vector<Point3> MOVE_DIRECTION_DELTA;
	vector<Point3> tmpUavScope_1; // @ in bool isUavInArea();
	vector<Point3> tmpUavMoveScope_2;
	vector<Point3> tmpUavMoveScope_3;
	vector<Point3> tmpPath; // @ in setMinUavHorizontalPath();
	vector<Point3> minPath; // @ in setMinUavHorizontalPath();
	vector<Point3> tmpGoodsPath; // @ in searchGoods();
	vector<Point3> minGoodsPath; // @ in searchGoods();
	vector<vector<vector<int>>> pathMap;
	vector<vector<vector<int>>> statusMap;
	vector<vector<vector<int>>> restoredMap;
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
	void updateHomeStatus();
	void updateWeUavMark(UAV &_uav);
	void changeUavAction(UAV_ACTION _from, UAV_ACTION _to);
	// environment-aware
	int environmentAware(UAV &_uav);
	bool isPathCross(const UAV &_dodgeUav, const Point3 &_uavPos, const Point3 &_uavNextPos);
	bool isPathCross(const Point3 &_pA1, const Point3 &_pA2, const Point3 &_pB1, const Point3 &_pB2);
	template<typename T>
	int isUavInArea(const Point3 &_p, vector<int> &_uavNo, T &_area);
	bool isUavInArea(const Point3 &_p, int & _ignoredNo, CHECK_OPT _opt = CHECK_OPT::CO_ALL);
	int fixStaticDodgeArea(list<Point3>& _area, UAV & _dodgeUav, UAV & _uav, const Point3 & _center);
	int fixDynamicDodgeArea(list<Point3>& _area, UAV & _dodgeUav, UAV & _uav, const Point3 & _center);
	void removePointInDodgeArea(list<Point3> &_area, const Point3 &_removedPoint);
	void doubleCheckDodgeArea(list<Point3>& _area, int &_ignoredNo, CHECK_OPT _opt = CHECK_OPT::CO_ALL);
	Point3 getBestDodgePositon(list<Point3> &_area, UAV &_uav);
	bool isPositionInMap(const Point3 &_p);
	int getDistanceInScope(const Point3 &_p1, const Point3 &_p2);
	int getHorizontalDistance(const Point3 &_p1, const Point3 &_p2);
	void uavDodgeAndGetNewPath(UAV &_uav, Point3 &_dodgeDirection);
	UAV *getUavPtrByNo(int _No);
	template<typename T>
	void getUavMoveScope(const Point3  &_center, T& _scope); // @ the center point is not included
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
	void initUavInfo();
	int getUavValue(UAV &_uav);
	void getUavInfo(UAV &_uav);
	int getBuyNewUavIndex(GOODS &_goods);
	// search goods
	void searchGoods();
	void updateGoodsDeliveryingPath();
	int getGoodsIndexByNo(int _No);
	// attacker
	void setAttackTarget();
	void updateAttackTarget();
	// power
	void updatePowerInfo();
	void updateChargingFlag();
	void chargeUav(UAV &_uav);
	void dischargeUav(UAV &_uav);
};

#endif // !__UAVAI_H__
