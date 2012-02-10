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

		void set_ambient_color(D3DXVECTOR3 color) { _ambient_color = color; }
		void set_diffuse_color(D3DXVECTOR3 color) { _diffuse_color = color; }
		void set_specular_color(D3DXVECTOR3 color) { _specular_color = color; _specular_intensity = (color.x + color.y + color.z)/3.0f; }

		void set_specular(bool specular) { _specular = specular; }
		bool is_specular() const { return _specular; }

		void set_alpha(float alpha) { _alpha = alpha; }
		float get_alpha() const { return _alpha; }

		void set_specular_power(float power) { _specular_power = power; }
		float get_specular_power() const { return _specular_power; }

		void set_specular_intensity(float intensity) { _specular_intensity = intensity; }
		float get_specular_intensity() const { return _specular_intensity; }

		D3DXVECTOR3 get_ambient_color() const { return _ambient_color; }
		D3DXVECTOR3 get_diffuse_color() const { return _diffuse_color; }
		D3DXVECTOR3 get_specular_color() const { return _specular_color; }

		bool has_texture() const { return _textureRV != NULL; }
		ID3D10ShaderResourceView* get_texture() const { return _textureRV; }
		bool create_texture(ID3D10Device *device, std::wstring filename);
		bool create_texture_from_tga(ID3D10Device *device, std::wstring filename);

		void set_technique(std::string tech){ _tech_name = tech; }
		const std::string & get_technique() const { return _tech_name; }

	private:
		float _specular_power;
		float _specular_intensity;

		float _alpha;
		bool _specular;

		D3DXVECTOR3 _ambient_color;
		D3DXVECTOR3 _diffuse_color;
		D3DXVECTOR3 _specular_color;

		ID3D10ShaderResourceView* _textureRV;

		std::string _tech_name;
	};
}

#endif