// 虽然叫A*但是只是用贪心算法确保每一步都是里目标最近的，想做A*但是时间有限

#ifndef __ASTAR_H__
#define __ASTAR_H__

#include "Point3.h"
#include <math.h>
#include <iostream>
#include <vector>

using std::vector;
using std::pow;

void setMarkInMap(vector<vector<vector<int>>> &_map, const Point3 &_p, const int &_mark);
int getMarkInMap(vector<vector<vector<int>>> &_map, const Point3 &_p);

// 搜索点
class SearchPoint
{
public:
	SearchPoint(const Point3 &_p, Point3 *_to);
	SearchPoint() {};
	~SearchPoint() { to = NULL; };
	Point3 p;
	Point3 *to = NULL;
	int distance = -1;
	void setParameters(const Point3 &_p, Point3 *_to);
private:
	int getDistance(const Point3 &_from, const Point3 &_to);
};

// 搜索区域
class SearchArea
{
public:
	SearchArea(const Point3 &_parentPoint, Point3 *_to, vector<vector<vector<int>>> *_map);
	SearchArea() {};
	~SearchArea() { map = NULL; to = NULL; };
	Point3 parentPoint;
	Point3 nextPoint;
	int mapX;
	int mapY;
	bool isDeadPath = false;
	void setParameters(const Point3 &_parentPoint, Point3 *_to, vector<vector<vector<int>>> *_map);
private:
	Point3 * to = NULL;
	int CLOSE_FLAG = 1;
	vector<vector<vector<int>>> *map = NULL;
	vector<SearchPoint> area;
	void getAreaPoints();
	void getNextPoint();
	bool isValidPoint(Point3 &_p);
};

class AStar
{
public:
	AStar();
	~AStar();
	void setMapAndPoint(vector<vector<vector<int>>> *_map, const Point3 &_p1, const Point3 &_p2);
	bool getPath(vector<Point3> &_path, int &_pathLength);

private:
	unsigned int lastPathLength;
	int getDistance(const Point3 &_p1, const Point3 &_p2);
	Point3 from, to;
	vector<vector<vector<int>>> *map = NULL;
	vector<SearchArea> area;
	vector<Point3> markedPoints;
	void clearPathMarkInMap(const int &_length);
};

#endif