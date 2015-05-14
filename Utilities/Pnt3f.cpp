// CS559 Utility Code
// - implementation of simple 3D point class
// see the header file for details
//
// file put together by Mike Gleicher, October 2008
//

#include "Pnt3f.H"
#include "math.h"

Pnt3f::Pnt3f() : x(0), y(0), z(0)
{
}
Pnt3f::Pnt3f(const float* iv) : x(iv[0]), y(iv[1]), z(iv[2])
{
}
Pnt3f::Pnt3f(const float _x, const float _y, const float _z) : x(_x), y(_y), z(_z)
{
}

void Pnt3f::normalize()
{
	if (isZero()) {
		x = 0;
		y = 1;
		z = 0;
	} else {
		float l = norm();
		x /= l;
		y /= l;
		z /= l;
	}
}

float Pnt3f::norm() const
{
	return sqrt(x * x + y * y + z * z);
}

bool Pnt3f::isZero() const
{
	return norm() < 1e-3;
}
// This code tells us where the original came from in CVS
// Its a good idea to leave it as-is so we know what version of
// things you started with
// $Header: /p/course/cs559-gleicher/private/CVS/Utilities/Pnt3f.cpp,v 1.3 2008/10/19 01:54:28 gleicher Exp $