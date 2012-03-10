#ifndef _HUD_H
#define _HUD_H

#include "DXUTgui.h"
#include "SDKmisc.h"

namespace Deferred
{
	class Hud
	{
	public:
		Hud(ID3D10Device *device);
		~Hud();

		void render(UINT poly_count);

	private:
		ID3D10Device *_device;
		LPD3DX10FONT _font;
		LPD3DX10SPRITE _sprite;
		RECT _rect;
	};
}
#endif