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
	vector<vector<vector<int>>> pathMap; // 用于路径规划的地图
	vector<vector<vector<int>>> statusMap; // 用于移动时环境感知的地图
	vector<vector<vector<int>>> restoredMap; // 恢复标记的地图
	vector<GOODSSTATUS> goodsStatus;
	AStar pathSearcher;
	// 初始化UAV位置
	void initUavLastPos();
	// 复制UAV
	void copyUav(const UAV &_src, UAV &_dst);
	// 获取地图值
	int getMapValue(const vector<vector<vector<int>>> &_array, const Point3 &_p);
	// 设置地图值
	void setMapValue(vector<vector<vector<int>>> &_array, const Point3 &_p, int _v);
	// 填充区域
	void fillArea(vector<vector<vector<int>>> &_Array, const Point3 &_p1, const Point3 &_p2, int _fill);
	// 重置UAV移动Flag
	void resetUavMovedFlag();
	// 移动
	void moving(UAV &_uav);
	// 移动指定状态的UAV
	void moveAllUavByAction(UAV_ACTION _action, int &_uavNum);
	// 更新停机坪状态
	void updateHomeStatus();
	// 更新我方UAV地图标记
	void updateWeUavMark(UAV &_uav);
	// 改变UAV Action
	void changeUavAction(UAV_ACTION _from, UAV_ACTION _to);
	// 环境感知
	int environmentAware(UAV &_uav);
	// 检测路径是否交叉（防止撞机）
	bool isPathCross(const UAV &_dodgeUav, const Point3 &_uavPos, const Point3 &_uavNextPos);
	bool isPathCross(const Point3 &_pA1, const Point3 &_pA2, const Point3 &_pB1, const Point3 &_pB2);
	// 检测区域内是否有UAV
	template<typename T>
	int isUavInArea(const Point3 &_p, vector<int> &_uavNo, T &_area);
	bool isUavInArea(const Point3 &_p, int & _ignoredNo, CHECK_OPT _opt = CHECK_OPT::CO_ALL);
	// 修正静止UAV的避让区域
	int fixStaticDodgeArea(list<Point3>& _area, UAV & _dodgeUav, UAV & _uav, const Point3 & _center);
	// 修正移动UAV的避让区域
	int fixDynamicDodgeArea(list<Point3>& _area, UAV & _dodgeUav, UAV & _uav, const Point3 & _center);
	// 删除避让区域中的某个点
	void removePointInDodgeArea(list<Point3> &_area, const Point3 &_removedPoint);
	// 双步检测避让区域
	void doubleCheckDodgeArea(list<Point3>& _area, int &_ignoredNo, CHECK_OPT _opt = CHECK_OPT::CO_ALL);
	// 寻找最佳避让位置
	Point3 getBestDodgePositon(list<Point3> &_area, UAV &_uav);
	// 位置是否在地图上
	bool isPositionInMap(const Point3 &_p);
	// 获取域中的距离（域中某点与目标距离）
	int getDistanceInScope(const Point3 &_p1, const Point3 &_p2);
	// 获取水平距离
	int getHorizontalDistance(const Point3 &_p1, const Point3 &_p2);
	// 躲避和获取新路径
	void uavDodgeAndGetNewPath(UAV &_uav, Point3 &_dodgeDirection);
	// 根据No获取UAV指针
	UAV *getUavPtrByNo(int _No);
	// 获取UAV移动域
	template<typename T>
	void getUavMoveScope(const Point3  &_center, T& _scope); // @ the center point is not included
	// 路径规划
	bool getPath(UAV &_uav);
	bool getPath(const Point3 &_from, const Point3 &_to, vector<Point3> &_path, int &_pathLength);
	// 获取垂直方向路径
	bool setUavVirticalPath(const Point3 & _from, const Point3 & _to, vector<Point3> &_path, int &_pathLength);
	// 获取水平方向最小路径
	bool setMinUavHorizontalPath(const Point3 & _from, const Point3 & _to, vector<Point3> &_path, int &_pathLength);
	// 获取某个高度上的水平路径
	bool getHorizontalPath(const Point3 &_from, const Point3 &_to, const int &_z, vector<Point3> &_path, int &_pathLength);
	// 清除UAV路径
	void clearUavPath(UAV &_uav);
	// 买入新UAV
	void buyNewUav();
	void initUavInfo();
	int getUavValue(UAV &_uav);
	void getUavInfo(UAV &_uav);
	int getBuyNewUavIndex(GOODS &_goods);
	// 搜索GOODS
	void searchGoods();
	void updateGoodsDeliveryingPath();
	int getGoodsIndexByNo(int _No);
	// Attacker
	void setAttackTarget();
	void updateAttackTarget();
	// Power
	void updatePowerInfo();
	void updateChargingFlag();
	void chargeUav(UAV &_uav);
	void dischargeUav(UAV &_uav);
};

#endif // !__UAVAI_H__
