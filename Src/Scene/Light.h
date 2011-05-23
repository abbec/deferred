#ifndef _LIGHT_H
#define _LIGHT_H

namespace Deferred
{
	class Light
	{
	public:
		const enum types{DIRECTIONAL = 1, POINT = 2};

		Light(D3DXVECTOR4 color, D3DXVECTOR3 position) : _color(color)
			, _position(position) {}

		virtual ~Light(){}

		D3DXVECTOR4 get_color() { return _color; }
		D3DXVECTOR3 get_position() { return _position; }
		virtual UINT get_type() = 0;

	private:
		D3DXVECTOR4 _color;
		D3DXVECTOR3 _position;
	};

}
#endif