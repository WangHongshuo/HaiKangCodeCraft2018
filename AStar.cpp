#include "AStar.h"

AStar::AStar()
{
}


AStar::~AStar()
{
	map = NULL;
}

void AStar::setMapAndPoint(vector<vector<vector<int>>>* _map, const Point3 & _p1, const Point3 & _p2)
{
	map = _map;
	from = _p1;
	to = _p2;

}

bool AStar::getPath(vector<Point3>& _path, int &_pathLength)
{
	lastPathLength = _path.size();
	area.resize(1);
	area[0].setParameters(from, &to, map);
	SearchArea _tmpArea;
	unsigned int _index = 0;
	while (area[_index].nextPoint != to)
	{
		// 如果是死路，恢复path，返回false
		if (area[_index].isDeadPath)
		{
			clearPathMarkInMap(_index + 1);
			return false;
		}
		_tmpArea.setParameters(area[_index].nextPoint, &to, map);
		area.push_back(_tmpArea);
		_index++;
	}
	_path.resize(lastPathLength + _index + 1);
	for (unsigned int i = lastPathLength; i < _path.size(); i++)
	{
		_path[i] = area[i - lastPathLength].nextPoint;
	}
	_pathLength = _path.size();
	clearPathMarkInMap(_index + 1);
	return true;
}

int AStar::getDistance(const Point3 & _p1, const Point3 & _p2)
{
	int _distance;
	_distance = int(pow(_p1.x - _p2.x, 2) + pow(_p1.y - _p2.y, 2));
	return _distance;
}

void AStar::clearPathMarkInMap(const int &_length)
{
	int _mark = 0;
	for (int i = 0; i < _length; i++)
	{
		setMarkInMap(*map, area[i].nextPoint, _mark);
	}
}


SearchPoint::SearchPoint(const Point3 & _p, Point3 * _to)
{
	p = _p;
	to = _to;
	distance = getDistance(p, *to);
}

void SearchPoint::setParameters(const Point3 & _p, Point3 * _to)
{
	p = _p;
	to = _to;
	distance = getDistance(p, *to);
}

int SearchPoint::getDistance(const Point3 & _from, const Point3 & _to)
{
	int _distance;
	_distance = int(pow(_from.x - _to.x, 2) + pow(_from.y - _to.y, 2));
	return _distance;
}

SearchArea::SearchArea(const Point3 & _parentPoint, Point3 * _to, vector<vector<vector<int>>>* _map)
{
	parentPoint = _parentPoint;
	to = _to;
	map = _map;
	area.resize(0);
	mapX = _map->size();
	mapY = (*_map)[0].size();
	isDeadPath = false;
	getAreaPoints();
	getNextPoint();
}

void SearchArea::setParameters(const Point3 & _parentPoint, Point3 * _to, vector<vector<vector<int>>>* _map)
{
	parentPoint = _parentPoint;
	to = _to;
	map = _map;
	area.resize(0);
	mapX = _map->size();
	mapY = (*_map)[0].size();
	isDeadPath = false;
	getAreaPoints();
	getNextPoint();
}

void SearchArea::getAreaPoints()
{
	Point3 _tmpPoint = parentPoint;
	SearchPoint _tmpSearchPoint;
	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			_tmpPoint.x += i;
			_tmpPoint.y += j;
			if (isValidPoint(_tmpPoint))
			{
				if (_tmpPoint == parentPoint || getMarkInMap(*map,_tmpPoint) == 1)
					continue;
				else
				{
					_tmpSearchPoint.setParameters(_tmpPoint, to);
					area.push_back(_tmpSearchPoint);
				}
			}
			_tmpPoint = parentPoint;
		}
	}
}

void SearchArea::getNextPoint()
{
	int _minDistance = 999999;
	unsigned int _minDistanceIndex;
	bool _isGetPoint = false;
	for (unsigned int i = 0; i < area.size(); i++)
	{
		if (area[i].distance < _minDistance)
		{
			_minDistance = area[i].distance;
			_minDistanceIndex = i;
			_isGetPoint = true;
		}
	}
	if (_isGetPoint)
	{
		nextPoint = area[_minDistanceIndex].p;
		setMarkInMap(*map, nextPoint, MARK);
	}
	else
	{
		isDeadPath = true;
	}
}

bool SearchArea::isValidPoint(Point3 & _p)
{
	if (_p.x > mapX - 1 || _p.y > mapY - 1 || _p.x < 0 || _p.y < 0)
		return false;
	else if (getMarkInMap(*map, _p) <= -3)
		return false;
	else
		return true;
}

void setMarkInMap(vector<vector<vector<int>>>& _map, const Point3 & _p, const int & _mark)
{
	_map[_p.x][_p.y][_p.z] = _mark;
}

int getMarkInMap(vector<vector<vector<int>>>& _map, const Point3 & _p)
{
	return _map[_p.x][_p.y][_p.z];
}
