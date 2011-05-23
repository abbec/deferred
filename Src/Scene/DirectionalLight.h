#ifndef _DIRECTIONAL_LIGHT_H
#define _DIRECTIONAL_LIGHT_H

#include "Light.h"

namespace Deferred
{
	class DirectionalLight : public Light
	{
	public:
		DirectionalLight(D3DXVECTOR4 color, D3DXVECTOR3 position, D3DXVECTOR3 direction) :
		  Light(color, position), _direction(direction) {}

		  ~DirectionalLight(){}

		  D3DXVECTOR3 get_direction() { return _direction; }
		  UINT get_type() { return Light::DIRECTIONAL; }

	private:
		D3DXVECTOR3 _direction;
	};
}

#endif