#ifndef _DIRECTIONAL_LIGHT_H
#define _DIRECTIONAL_LIGHT_H

#include "Light.h"

namespace Deferred
{
	class DirectionalLight : public Light
	{
	public:
		DirectionalLight(D3DXVECTOR4 color, D3DXVECTOR3 position, D3DXVECTOR3 direction) :
		  Light(color), _position(position), _direction(direction) {}

		  D3DXVECTOR3 get_position() { return _position; }
		  D3DXVECTOR3 get_direction() { return _direction; }

	private:
		D3DXVECTOR3 _position;
		D3DXVECTOR3 _direction;
	};
}

#endif