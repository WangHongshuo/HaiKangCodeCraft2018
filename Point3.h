#ifndef __POINT3_H__
#define __POINT3_H__

#include <iostream>
#include <vector>

class Point3
{
public:
	Point3(int _x, int _y, int _z) { x = _x; y = _y; z = _z; }
	Point3() {}
	~Point3() {}
	int x = -1;
	int y = -1;
	int z = -1;
	void setPoint(int _x, int _y, int _z) { x = _x; y = _y; z = _z; }
	void setPoint(const Point3 &_p);
	Point3 &operator=(const Point3 &_p);
	Point3 operator+(const Point3 &_p);
	friend Point3 operator+(const Point3 &_p1, Point3 &_p2);
	Point3 operator-(const Point3 &_p);
	friend Point3 operator-(const Point3 &_p1, Point3 &_p2);
	bool operator==(const Point3 &_p);
	bool operator!=(const Point3 &_p);

private:
};
#endif // !__POINT3_H__
