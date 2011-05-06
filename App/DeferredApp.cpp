#include "DXUT.h"
#include "DeferredApp.h"
#include <iostream>

DeferredApp *DeferredApp::inst = NULL;

DeferredApp::DeferredApp() :
_d3d_device(NULL), _time(0.0), 
_elapsed_time(0.0), _user_context(NULL)
{

}

DeferredApp *DeferredApp::instance()
{
	if (!inst)
		inst = new DeferredApp();

	return inst;
}

void DeferredApp::render(ID3D10Device* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext)
{
	_d3d_device = pd3dDevice;
	_time = fTime;
	_elapsed_time = fElapsedTime;
	_user_context = pUserContext;

	// Clear render target and the depth stencil 
    float ClearColor[4] = { 0.176f, 0.196f, 0.667f, 0.0f };
    pd3dDevice->ClearRenderTargetView( DXUTGetD3D10RenderTargetView(), ClearColor );
    pd3dDevice->ClearDepthStencilView( DXUTGetD3D10DepthStencilView(), D3D10_CLEAR_DEPTH, 1.0, 0 );

	// Fill G-buffers
	geometry_stage();


	// P-buffers


	//Final composition
}

void DeferredApp::geometry_stage()
{

}