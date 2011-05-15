#include "DXUT.h"
#include "DXUTShapes.h"
#include "Scene.h"
#include <cassert>
#include <iostream>
#include <conio.h>

Scene::Scene() :
_sphere(NULL), _worldVariable(NULL),
_viewVariable(NULL), _projectionVariable(NULL),
_teapot(NULL)
{
	
}

Scene::~Scene()
{
	// Clean up scene
	if (_sphere != NULL)
		_sphere->Release();

	if (_teapot != NULL)
		_teapot->Release();
}

HRESULT Scene::init(ID3D10Device *device, ID3D10Effect *effect)
{
	RECT rc;
    GetClientRect( DXUTGetHWND(), &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

	HRESULT hr;

    // Obtain the variables
    _worldVariable = effect->GetVariableByName( "World" )->AsMatrix();
    _viewVariable = effect->GetVariableByName( "View" )->AsMatrix();
    _projectionVariable = effect->GetVariableByName( "Projection" )->AsMatrix();
	_wv_inverse = effect->GetVariableByName( "WorldViewInverse" )->AsMatrix();
	_spec_intensity_var = effect->GetVariableByName( "SpecularIntensity" )->AsScalar();
	_texture_SR = effect->GetVariableByName("AlbedoTexture")->AsShaderResource();

	// Create meshes
	DXUTCreateSphere(device, 1.0, 25, 25, &_sphere);
	DXUTCreateBox(device, 1.0, 1.0, 1.0, &_teapot);

	if (!_object.read_from_obj(device, "Media\\monkey.obj"))
		_cprintf("Error in initializing OBJ object! \n");

	// TODO: Set up lighting

	_sphere->CommitToDevice();
	_teapot->CommitToDevice();

	// Initialize the world matrix
    D3DXMatrixIdentity( &_world );

    //// Initialize the view matrix
    //D3DXVECTOR3 Eye( 2.0f, 1.0f, 2.0f );
    //D3DXVECTOR3 At( 0.0f, 0.0f, 0.0f );
    //D3DXVECTOR3 Up( 0.0f, 1.0f, 0.0f );
    //D3DXMatrixLookAtRH( &_view, &Eye, &At, &Up );

    //// Initialize the projection matrix
    //D3DXMatrixPerspectiveFovRH( &_projection, ( float )D3DX_PI * 0.5f, width / ( FLOAT )height, 0.1f, 100.0f );

	D3DXVECTOR3 Eye( 2.0f, 1.0f, 2.0f );
    D3DXVECTOR3 At( 0.0f, 0.0f, 0.0f );
	_camera.SetViewParams(&Eye, &At);
	_camera.SetProjParams(( float )D3DX_PI * 0.5f, width / ( float )height, 0.1f, 100.0f);
	_camera.SetWindow(width, height);

	return S_OK;
}

void Scene::update(double fTime, float fElapsedTime, void* pUserContext)
{
	_camera.FrameMove(fElapsedTime);
}

LRESULT Scene::handle_messages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return _camera.HandleMessages(hWnd, uMsg, wParam, lParam);
}

void Scene::bump_shader_variables()
{
    // Update variables
	_world = *_camera.GetWorldMatrix();
	_view = *_camera.GetViewMatrix();
	_projection = *_camera.GetProjMatrix();

    _worldVariable->SetMatrix( ( float* )&_world );
    _viewVariable->SetMatrix( ( float* )&_view );
    _projectionVariable->SetMatrix( ( float* )&_projection );

	D3DXMATRIX world_view;

	world_view = _world * _view;
	D3DXMatrixInverse( &_world_view_inv, NULL, &world_view);
	D3DXMatrixTranspose(&_world_view_inv, &_world_view_inv);
	_wv_inverse->SetMatrix((float *)&_world_view_inv);
	_spec_intensity_var->SetFloat(0.8);
}

void Scene::render(ID3D10Device *device)
{
		//_teapot->DrawSubset(0);
		_texture_SR->SetResource(_object.get_texture());
		_object.render();
}