#ifndef _BOUNDING_FRUSTUM_H
#define _BOUNDING_FRUSTUM_H

namespace Deferred
{

struct Plane
{
	D3DXVECTOR3 normal;
	float d;
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