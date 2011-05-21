#ifndef _LIGHT_H
#define _LIGHT_H

namespace Deferred
{
	class Light
	{
	public:
		const enum types{DIRECTIONAL = 1, POINT = 2};

		Light(D3DXVECTOR4 color, D3DXVECTOR3 position, UINT type = DIRECTIONAL) : _color(color)
			, _type(type), _position(position) {}

		virtual ~Light(){}

		D3DXVECTOR4 get_color() { return _color; }
		D3DXVECTOR3 get_position() { return _position; }
		UINT get_type() { return _type; }

	private:
		D3DXVECTOR4 _color;
		D3DXVECTOR3 _position;
		UINT _type;
	};

}
#endif