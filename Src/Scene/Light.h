#ifndef _LIGHT_H
#define _LIGHT_H

namespace Deferred
{
	class Light
	{
	public:
		const enum types{DIRECTIONAL = 1, POINT = 2};

		Light(D3DXVECTOR4 color, UINT type = DIRECTIONAL) : _color(color), _type(type) {}

		D3DXVECTOR4 get_color() { return _color; }
		UINT get_type() { return _type; }

	private:
		D3DXVECTOR4 _color;
		UINT _type;
	};

}
#endif