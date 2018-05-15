#include "Point3.h"

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
