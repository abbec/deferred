#ifndef _BOUNDING_FRUSTUM_H
#define _BOUNDING_FRUSTUM_H

namespace Deferred
{

struct Plane
{
	D3DXVECTOR3 normal;
	float d;

	inline void normalize()
	{
		float denom = 1 / sqrt((normal.x*normal.x) + (normal.y*normal.y) + (normal.z*normal.z));
		normal.x = normal.x * denom;
		normal.y = normal.y * denom;
		normal.z = normal.z * denom;
		d = d * denom;

	}
};

class BoundingFrustum
{

public:
	
	BoundingFrustum(D3DXMATRIX matrix) : _matrix(matrix) {}
	D3DXVECTOR3* get_corners();

private:

	D3DXMATRIX _matrix;
	D3DXVECTOR3 intersect_3_planes(Plane p1, Plane p2, Plane p3);

};

}
#endif