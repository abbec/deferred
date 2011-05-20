#ifndef _POINT_LIGHT_H
#define _POINT_LIGHT_H

#include "Light.h"

namespace Deferred
{
	class PointLight : public Light
	{
	public:
		PointLight(D3DXVECTOR4 color, D3DXVECTOR3 position) : Light(color, Light::POINT),
			_position(position) {}  

		D3DXVECTOR3 get_position() { return _position; }

	private:
		D3DXVECTOR3 _position;
	};
}

#endif