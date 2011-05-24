#include "DXUT.h"
#include "Hud.h"

using namespace Deferred;

Hud::Hud(ID3D10Device *device) : _device(device)
{
	// Create a font
	D3DX10CreateFontW( _device, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                L"Consolas", &_font);

	D3DX10CreateSprite(_device, 512, &_sprite);

}

Hud::~Hud()
{
	SAFE_RELEASE(_font);
	SAFE_RELEASE(_sprite);
}

void Hud::render()
{
	ID3D10DepthStencilState *old_dss;
	_device->OMGetDepthStencilState(&old_dss, 0);

	FLOAT OriginalBlendFactor[4];   
    UINT OriginalSampleMask = 0;   
    ID3D10BlendState* pFontBlendState10=NULL;
	D3D10_BLEND_DESC StateDesc;
	ZeroMemory( &StateDesc, sizeof( D3D10_BLEND_DESC ) );
	StateDesc.AlphaToCoverageEnable = FALSE;
	StateDesc.BlendEnable[0] = TRUE;
	StateDesc.SrcBlend = D3D10_BLEND_SRC_ALPHA;
	StateDesc.DestBlend = D3D10_BLEND_SRC_ALPHA;
	StateDesc.BlendOp = D3D10_BLEND_OP_ADD;
	StateDesc.SrcBlendAlpha = D3D10_BLEND_ONE;
	StateDesc.DestBlendAlpha = D3D10_BLEND_ONE;
	StateDesc.BlendOpAlpha = D3D10_BLEND_OP_MAX;
	StateDesc.RenderTargetWriteMask[0] = 0xf;
	_device->CreateBlendState( &StateDesc, &pFontBlendState10 );// for blending   
    ID3D10BlendState* pOriginalBlendState10=NULL;  
  
    // Save the current blend state   
    _device->OMGetBlendState(&pOriginalBlendState10, OriginalBlendFactor, &OriginalSampleMask);   
    // Set the blend state for font drawing   
    if(pFontBlendState10) {
        FLOAT NewBlendFactor[4] = {1,0,0,0};   
        _device->OMSetBlendState(pFontBlendState10, NewBlendFactor, 0xf);   
    }   
       
    // Create and Initialize the destination rectangle   
    RECT rc; 
	rc.bottom = 16;
	rc.left = 0;
	rc.right = 200;
	rc.top = 0;
	RECT rc2; 
	rc2.bottom = 32;
	rc2.left = 0;
	rc2.right = 200;
	rc2.top = 16;

    // Start font drawing   
    _sprite->Begin(0);
    // Draw the text to the screen
    HRESULT hr = _font->DrawTextW( _sprite, DXUTGetFrameStats(true), -1, &rc, DT_LEFT | DT_NOCLIP, D3DCOLOR_ARGB(255, 0, 255, 0));   
	hr = _font->DrawTextW( _sprite, DXUTGetDeviceStats(), -1, &rc2, DT_LEFT | DT_NOCLIP, D3DCOLOR_ARGB(255, 0, 255, 0));
    _sprite->End();
    // Restore the previous blend state and depth test
	_device->OMSetBlendState(pOriginalBlendState10, OriginalBlendFactor, OriginalSampleMask);
	_device->OMSetDepthStencilState(old_dss, 0);

	SAFE_RELEASE(old_dss);
	SAFE_RELEASE(pFontBlendState10);
	SAFE_RELEASE(pOriginalBlendState10);

}