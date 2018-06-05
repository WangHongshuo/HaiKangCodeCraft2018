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
	enum MOVE_DIRECTION { M_UPWARD, M_DOWNWARD, M_HORIZONTAL };
	enum CHECK_OPT { CO_ALL, CO_ALLY, CO_ENEMY };
	void initPtr(MAP_INFO * _map, MATCH_STATUS * _match, FLAY_PLANE * _flayPlane);
	void initMap();
	void setInitUavTarget();
	void getNextAction();
private:
	static const int MOVE_DIRECTION_NUM = 10;
	int MAX_PATH_HEIGHT = -1;
	int MAX_ALIVE_UAV_NUM;
	int MAX_ATTACKER_UAV_NUM;
	int money = 0;
	int UAVValues = 0;
	int UAVAliveNum = 0;
	int UAVAttackerNum = 0;
	int UAVNum = -1;
	int initUavValueNum = 0;
	int tmpMark_1 = -1; // @ in environmentAware();
	int tmpMark_2 = -1; // @ in updateWeUavMark();
	int moveAction = -1;
	int cheapestUavIndex = 0;
	int mostExpensiveUavIndex = 0;
	int uavMoveDirection = -1;
	bool isInitPtr = false;
	UAV *pEnemyUav = NULL;
	UAV *pAllyUav = NULL;
	MAP_INFO *map = NULL;
	MATCH_STATUS *match = NULL;
	FLAY_PLANE *plan = NULL;
	Point3 tmpPoint_1; // @ in moving();
	Point3 tmpPoint_2; // @ in moving();
	Point3 tmpPoint_3; // @ in isEnemyUavInArea();
	Point3 tmpPoint_4; // @ in environmentAware();
	Point3 tmpPoint_5; // @ in getUavMoveScope();
	Point3 enemyParkingPos;
	vector<int> uavNo;
	list<Point3> dodgePosition; // @ in environmentAware();
	list<Point3>::iterator it_1; // @ in fixDodgeArea();
	list<Point3>::iterator it_2; // @ in doubleCheckDodgeArea();
	list<Point3>::iterator it_3; // @ in getBestDodgePositon();
	vector<Point3> MOVE_DIRECTION_DELTA;
	vector<Point3> tmpUavScope_1; // @ in bool isUavInArea();
	vector<Point3> tmpUavMoveScope_2;
	vector<Point3> tmpUavMoveScope_3;
	vector<Point3> tmpPath; // @ in setMinUavHorizontalPath();
	vector<Point3> minPath; // @ in setMinUavHorizontalPath();
	vector<Point3> tmpGoodsPath; // @ in searchGoods();
	vector<Point3> minGoodsPath; // @ in searchGoods();
	vector<Point3> availablePoints; // @ in getAvailableAreaPosisiton
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
	void updateWeUavMark(UAV &_uav);
	// environment-aware
	int environmentAware(UAV &_uav);
	int getMoveDirection(UAV &_uav);
	template<typename T>
	int isUavInArea(const Point3 &_p, vector<int> &_uavNo, T &_area);
	bool isUavInArea(const Point3 &_p, int & _ignoredNo, CHECK_OPT _opt = CHECK_OPT::CO_ALL);
	int fixDodgeArea(list<Point3>& _area, UAV & _dodgeUav, UAV & _uav);
	void fixDodgeArea(list<Point3> &_area, const Point3 &_removedPoint);
	void doubleCheckDodgeArea(list<Point3>& _area, int &_ignoredNo, CHECK_OPT _opt = CHECK_OPT::CO_ALL);
	Point3 getBestDodgePositon(list<Point3> &_area, UAV &_uav);
	bool isPositionInMap(const Point3 &_p);
	int getDistanceInScope(const Point3 &_p1, const Point3 &_p2);
	int getHorizontalDistance(const Point3 &_p1, const Point3 &_p2);
	Point3 getAvailableAreaPosisiton(const Point3 &_p, UAV &_uav);
	void uavDodgeAndGetNewPath(UAV &_uav, Point3 &_dodgeDirection);
	int getEnemyUavIndexByNo(const int &_No);
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
	void initUavValue();
	int getUavValue(UAV &_uav);
	int getBuyNewUavIndex(GOODS &_goods);
	// search goods
	void searchGoods();
	int getGoodsIndexByNo(int _No);
	// attacker
	void setAttackTarget();
	void updateAttackTarget();
};

#endif // !__UAVAI_H__
