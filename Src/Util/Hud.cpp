#include "DXUT.h"
#include "Hud.h"

using namespace Deferred;

Hud::Hud(ID3D10Device *device) : _device(device)
{
	// Create a font
	D3DX10CreateFontW( _device, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                L"Arial", &_font);

	D3DX10CreateSprite(_device, 512, &_sprite);

}

Hud::~Hud()
{

}

void Hud::render()
{
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
	rc.bottom = 20;
	rc.left = 0;
	rc.right = 20;
	rc.top = 0;
       
    // Start font drawing   
    _sprite->Begin(0);
    // Draw the text to the screen   
    HRESULT hr = _font->DrawText( _sprite, L"Hej", -1, &rc, DT_LEFT, D3DCOLOR_ARGB(255, 0, 255, 0));   



    _sprite->End();
    // Restore the previous blend state   
	_device->OMSetBlendState(pOriginalBlendState10, OriginalBlendFactor, OriginalSampleMask); 

}