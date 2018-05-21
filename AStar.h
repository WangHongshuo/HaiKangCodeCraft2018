#ifndef __ASTAR_H__
#define __ASTAR_H__

#include "Point3.h"
#include <math.h>
#include <iostream>
#include <vector>

using std::vector;
using std::pow;

// ËÑË÷µã
class SearchPoint
{
public:
	SearchPoint(const Point3 &_p, Point3 *_to);
	SearchPoint() {};
	~SearchPoint() { to = NULL; };
	Point3 p;
	Point3 *to = NULL;
	bool isOpen = true;
	int distance = -1;
	void setParameters(const Point3 &_p, Point3 *_to);
private:
	int getDistance(const Point3 &_from, const Point3 &_to);

};

// ËÑË÷ÇøÓò
class SearchArea
{
public:
	SearchArea(const Point3 &_parentPoint, const Point3 &_to, vector<vector<vector<int>>> *_map);
	SearchArea() {};
	~SearchArea() { map = NULL; };
	Point3 parentPoint;
	Point3 nextPoint;
	int mapX;
	int mapY;
	bool isDeadPath = false;
	void setParameters(const Point3 &_parentPoint, const Point3 &_to, vector<vector<vector<int>>> *_map);
private:
	Point3 to;
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

};

#endif