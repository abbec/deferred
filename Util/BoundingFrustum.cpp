#include "DXUT.h"

#include "BoundingFrustum.h"

using namespace Deferred;

D3DXVECTOR3* BoundingFrustum::get_corners()
{
	D3DXVECTOR3 *corners = new D3DXVECTOR3[4];

	// Extract planes from matrix
	Plane left, right, top, bottom, farp, nearp;

	// Left clipping plane
	left.normal[0] = _matrix._14 + _matrix._11; 
	left.normal[1] = _matrix._24 + _matrix._21; 
	left.normal[2] = _matrix._34 + _matrix._31; 
	left.d = _matrix._44 + _matrix._41;

	// Right clipping plane 
	right.normal[0] = _matrix._14 - _matrix._11; 
	right.normal[1] = _matrix._24 - _matrix._21; 
	right.normal[2] = _matrix._34 - _matrix._31; 
	right.d = _matrix._44 - _matrix._41;

	// Top clipping plane 
	top.normal[0] = _matrix._14 - _matrix._12; 
	top.normal[1] = _matrix._24 - _matrix._22; 
	top.normal[2] = _matrix._34 - _matrix._32; 
	top.d = _matrix._44 - _matrix._42;

	// Bottom clipping plane 
	bottom.normal[0] = _matrix._14 + _matrix._12; 
	bottom.normal[1] = _matrix._24 + _matrix._22; 
	bottom.normal[2] = _matrix._34 + _matrix._32; 
	bottom.d = _matrix._44 + _matrix._42;

	// Near clipping plane 
	nearp.normal[0] = _matrix._13; 
	nearp.normal[1] = _matrix._23; 
	nearp.normal[2] = _matrix._33; 
	nearp.d = _matrix._43;

	// Far clipping plane 
	farp.normal[0] = _matrix._14 - _matrix._13; 
	farp.normal[1] = _matrix._24 - _matrix._23; 
	farp.normal[2] = _matrix._34 - _matrix._33; 
	farp.d = _matrix._44 - _matrix._43; 


	// Find intersection points
	corners[0] = intersect_3_planes(left, top, farp);
	corners[1] = intersect_3_planes(left, bottom, farp);
	corners[2] = intersect_3_planes(right, bottom, farp);
	corners[3] = intersect_3_planes(right, top, farp);
}

D3DXVECTOR3 BoundingFrustum::intersect_3_planes(Plane p1, Plane p2, Plane p3)
{
	// Normalize vectors
	D3DXVec3Normalize(&p1.normal, &p1.normal);
	D3DXVec3Normalize(&p2.normal, &p2.normal);
	D3DXVec3Normalize(&p3.normal, &p3.normal);

	// Compute cross products
	D3DXVECTOR3 n2_x_n3;
	D3DXVec3Cross(&n2_x_n3, &p2.normal, &p3.normal);

	D3DXVECTOR3 n3_x_n1;
	D3DXVec3Cross(&n3_x_n1, &p3.normal, &p1.normal);

	D3DXVECTOR3 n1_x_n2;
	D3DXVec3Cross(&n1_x_n2, &p1.normal, &p2.normal);

	float n1_dot_n2_x_n3 = D3DXVec3Dot(&p1.normal, &n2_x_n3);

	return (p1.d*(n2_x_n3) + p2.d*(n3_x_n1) + p3.d*(n1_x_n2))/(n1_dot_n2_x_n3);
}