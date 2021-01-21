#pragma once
struct Vector2
{
	Vector2(double x_, double y_):
		x(x_), y(y_)
	{

	}
	double x;
	double y;
};
struct Vector3
{
	Vector3(double x_, double y_, double z_) :
		x(x_), y(y_), z(z_)
	{

	}
	Vector3(double v) :
		x(v), y(v), z(v)
	{

	}
	Vector3()
	{

	}
	double x;
	double y;
	double z;
};