#ifndef _MATERIAL_H
#define _MATERIAL_H

#include <iostream>

namespace Deferred
{
	class Material
	{
	public:
		Material();

		~Material();

		void set_ambient_color(D3DXVECTOR3 color){ _ambient_color = color; }
		void set_diffuse_color(D3DXVECTOR3 color){ _diffuse_color = color; }
		void set_specular_color(D3DXVECTOR3 color){ _specular_color = color; }

		void set_specular(bool specular){ _specular = specular; }
		bool get_specular(){ return _specular; }

		void set_alpha(float alpha){ _alpha = alpha; }
		float get_alpha(){ return _alpha; }

		void set_specular_power(float power){ _specular_power = power; }
		float get_specular_power(){ return _specular_power; }

		void set_specular_intensity(float intensity){ _specular_intensity = intensity; }
		float get_specular_intensity(){ return _specular_intensity; }

		D3DXVECTOR3 get_ambient_color(){ return _ambient_color; }
		D3DXVECTOR3 get_diffuse_color(){ return _diffuse_color; }
		D3DXVECTOR3 get_specular_color(){ return _specular_color; }

		bool create_texture(ID3D10Device *device, std::wstring filename);

	private:
		float _specular_power;
		float _specular_intensity;

		float _alpha;
		bool _specular;

		D3DXVECTOR3 _ambient_color;
		D3DXVECTOR3 _diffuse_color;
		D3DXVECTOR3 _specular_color;

		ID3D10ShaderResourceView* _textureRV;
	};
}

#endif