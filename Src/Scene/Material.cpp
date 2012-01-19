#include "DXUT.h"
#include "Material.h"
#include "FreeImage.h"
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

UINT GetNumMipLevels(UINT width, UINT height)
{
        UINT numLevels = 1;
        while(width > 1 || height > 1)
        {
        width = max(width / 2, 1);
        height = max(height / 2, 1);
        ++numLevels;
        }

        return numLevels;
}

bool Material::create_texture_from_tga(ID3D10Device *device, std::wstring filename)
{
	char path[256];
	wcstombs(path, filename.c_str(), -1);
	FIBITMAP *p_image = FreeImage_Load(FIF_TARGA, path);

	if (!p_image)
	{
		_cwprintf(L"Could not open targa texture %s\n", filename);
		return false;
	}

	//create a texture 2d description
	D3D10_TEXTURE2D_DESC descTex;
	ZeroMemory(&descTex, sizeof(descTex));
	descTex.Width = FreeImage_GetWidth(p_image);
	descTex.Height = FreeImage_GetHeight(p_image);

	//format of the unsigned char array
	descTex.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	
	//automatically generates mipmaps if this value is set according to documentation
	descTex.MipLevels = 1;
	descTex.ArraySize = 1;
	descTex.SampleDesc.Count = 1;
	descTex.SampleDesc.Quality = 0;
	descTex.Usage = D3D10_USAGE_DEFAULT;
	
	//needs to be set as a render target as well in order for hte direct3d device to call GenerateMipmaps on it
	descTex.BindFlags = D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_RENDER_TARGET;
	descTex.CPUAccessFlags = 0;
	
	//this flag needs to be set as well
	descTex.MiscFlags = D3D10_RESOURCE_MISC_GENERATE_MIPS;

	//subresource data (the image) that will be passed into the creation of the texture
	UINT mp = GetNumMipLevels(descTex.Width, descTex.Height);
	D3D10_SUBRESOURCE_DATA InitData[1];

	UINT bpp = FreeImage_GetBPP(p_image) / 8;

	for (int i=0; i < 1; ++i)
	{
		InitData[i].pSysMem = p_image->data;
		InitData[i].SysMemPitch = bpp * descTex.Width; // 4 for RGBA
		InitData[i].SysMemSlicePitch = 0;
	}

	ID3D10Texture2D* pTexture;
	HRESULT hr = device->CreateTexture2D(&descTex, InitData, &pTexture);

	if (FAILED(hr))
	{
		_cprintf("Could not create texture.");
		return false;
	}

	//create a shader resource view to view load the texture into a shader
	D3D10_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));

	srvDesc.Format = descTex.Format;
	srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;

	//I don't know what values to put for the Texture2D field
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	if (FAILED(device->CreateShaderResourceView(pTexture, &srvDesc, &_textureRV)))
	{
		_cwprintf(L"Could not create Targa texture\n.");
        return false;
	}

	return true;
}

bool Material::create_texture(ID3D10Device *device, std::wstring filename)
{
	HRESULT hr = D3DX10CreateShaderResourceViewFromFile(device, filename.c_str(), NULL, NULL, &_textureRV, NULL);
	if (FAILED(hr))
	{
		_cwprintf(L"Could not create texture from file: %s\n", filename.c_str());
		return false;
	}

	return true;
}