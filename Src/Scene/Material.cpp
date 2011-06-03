#include "DXUT.h"
#include "Material.h"

#include <conio.h>

using namespace Deferred;

Material::Material() : _ambient_color(D3DXVECTOR3( 0.2f, 0.2f, 0.2f )),
_diffuse_color(D3DXVECTOR3( 0.8f, 0.8f, 0.8f )), _specular_color(D3DXVECTOR3( 1.0f, 1.0f, 1.0f )),
_specular_power(100.0f), _alpha(1.0f), _specular(false), _textureRV(NULL), _tech_name("GeometryStageNoSpecularNoTexture")
{

}

Material::~Material()
{
	SAFE_RELEASE(_textureRV);
}

bool Material::create_texture(ID3D10Device *device, std::wstring filename)
{
	if (FAILED(D3DX10CreateShaderResourceViewFromFile(device, filename.c_str(), NULL, NULL, &_textureRV, NULL)))
	{
		_cwprintf(L"Could not create texture from file: %s\n", filename.c_str());
		return false;
	}

	return true;
}