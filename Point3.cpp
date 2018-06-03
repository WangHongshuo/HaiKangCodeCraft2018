#include "Point3.h"

void Point3::setPoint(const Point3 & _p)
{
	x = _p.x;
	y = _p.y;
	z = _p.z;
}

Point3 & Point3::operator=(const Point3 & _p)
{
	this->x = _p.x;
	this->y = _p.y;
	this->z = _p.z;
	return *this;
}

Point3 Point3::operator+(const Point3 & _p)
{
	Point3 _temp;
	_temp.x = this->x + _p.x;
	_temp.y = this->y + _p.y;
	_temp.z = this->z + _p.z;
	return _temp;
}

Point3 Point3::operator-(const Point3 & _p)
{
	Point3 _temp;
	_temp.x = this->x - _p.x;
	_temp.y = this->y - _p.y;
	_temp.z = this->z - _p.z;
	return _temp;
}

bool Point3::operator==(const Point3 & _p)
{
	if (this->x != _p.x || this->y != _p.y || this->z != _p.z)
		return false;
	else
		return true;
}

bool Point3::operator!=(const Point3 & _p)
{
	
	if (this->x != _p.x || this->y != _p.y || this->z != _p.z)
		return true;
	else
		return false;
}

Point3 operator+(const Point3 & _p1, Point3 & _p2)
{
	Point3 _temp;
	_temp.x = _p1.x + _p2.x;
	_temp.y = _p1.y + _p2.y;
	_temp.z = _p1.z + _p2.z;
	return _temp;
}

Point3 operator-(const Point3 & _p1, Point3 & _p2)
{
	Point3 _temp;
	_temp.x = _p1.x - _p2.x;
	_temp.y = _p1.y - _p2.y;
	_temp.z = _p1.z - _p2.z;
	return _temp;
}
