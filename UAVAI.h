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
	enum MOVE_ACTION { M_STANDBY, M_NEWPATH, M_NORMAL, M_MOVE_ALLY };
	enum MOVE_DIRECTION { M_UPWARD, M_DOWNWARD, M_HORIZONTAL};
	void initPtr(MAP_INFO * _map, MATCH_STATUS * _match, FLAY_PLANE * _flayPlane);
	void initMap();
	void setInitUavTarget();
	void getNextAction();
private:
	int MAX_ALIVE_UAV_NUM;
	int MAX_ATTACKER_UAV_NUM;
	int money = 0;
	int UAVAliveNum = 0;
	int UAVAttackerNum = 0;
	int UAVNum = -1;
	int initUavValueNum = 0;
	int tmpMark = -1; // @ in environmentAware();
	int moveAction = -1;
	int cheapestUavIndex = 0;
	int uavMoveDirection = -1;
	bool isInitPtr = false;
	UAV *uavAlly = NULL;
	MAP_INFO *map = NULL;
	MATCH_STATUS *match = NULL;
	FLAY_PLANE *plan = NULL;
	Point3 tmpPoint_1; // @ in moving();
	Point3 tmpPoint_2; // @ in moving();
	Point3 tmpPoint_3; // @ in isEnemyUavInArea();
	Point3 tmpPoint_4; // @ in environmentAware();
	vector<int> uavNo;
	vector<Point3> tmpPath; // @ in setMinUavHorizontalPath();
	vector<Point3> minPath; // @ in setMinUavHorizontalPath();
	vector<Point3> tmpGoodsPath; // @ in searchGoods();
	vector<Point3> minGoodsPath; // @ in searchGoods();
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
	void updateWeUavMark(UAV &_uav);
	// environment-aware
	int environmentAware(UAV &_uav);
	int getMoveDirection(UAV &_uav);
	int isEnemyUavInArea(const Point3 &_p, vector<int> &_uavNo);
	bool isPositionInMap(const Point3 &_p);
	Point3 getAvailableHorizontalAreaPosisiton(const Point3 &_p);
	void uavDodgeAndGetNewPath(UAV &_uav, Point3 &_dodgeDirection);
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
	bool isGetBetterGoods(UAV &_uav, GOODS &_goods);
	// attacker
	void setAttackTarget();
};

#endif // !__UAVAI_H__
