#ifndef _POINT_LIGHT_H
#define _POINT_LIGHT_H

#include "Light.h"

namespace Deferred
{
	class PointLight : public Light
	{
	public:
		PointLight(D3DXVECTOR4 color, D3DXVECTOR3 position) : Light(color, position) {}  

		UINT get_type() { return Light::POINT; }
	private:
	};
}

#endif