#include "DXUT.h"
#include "DXUTShapes.h"
#include "Scene.h"
#include <cassert>
#include <iostream>
#include <conio.h>
#include "..\Util\BoundingFrustum.h"

Scene::Scene() :
_worldVariable(NULL),
_viewVariable(NULL), _projectionVariable(NULL)
{
	
}

Scene::~Scene()
{

	std::vector<Deferred::Object *>::iterator it = _objects.begin();

	while (it != _objects.end())
	{
		Deferred::Object *o = *it;
		delete o;

		++it;
	}
}

HRESULT Scene::init(ID3D10Device *device, ID3D10Effect *effect)
{
	RECT rc;
    GetClientRect( DXUTGetHWND(), &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    // Obtain the variables
    _worldVariable = effect->GetVariableByName( "World" )->AsMatrix();
    _viewVariable = effect->GetVariableByName( "View" )->AsMatrix();
    _projectionVariable = effect->GetVariableByName( "Projection" )->AsMatrix();
	_wv_inverse = effect->GetVariableByName( "WorldViewInverse" )->AsMatrix();
	_spec_intensity_var = effect->GetVariableByName( "SpecularIntensity" )->AsScalar();
	_texture_SR = effect->GetVariableByName("AlbedoTexture")->AsShaderResource();
	_far_plane_corners_variable = effect->GetVariableByName("FarPlaneCorners")->AsVector();

	// Objects
	Deferred::Object *obj = new Deferred::Object();
	if (!obj->read_from_obj(device, "Media\\viking.obj"))
		_cprintf("Error in initializing OBJ object! \n");

	_objects.push_back(obj);

	// TODO: Set up lighting

	// Initialize the world matrix
    D3DXMatrixIdentity( &_world );

	// Set up a ModelView Camera
	D3DXVECTOR3 Eye( 2.0f, 1.0f, 2.0f );
    D3DXVECTOR3 At( 0.0f, 0.0f, 0.0f );
	_camera.SetViewParams(&Eye, &At);
	_camera.SetProjParams(( float )D3DX_PI * 0.5f, width / ( float )height, 0.1f, 100.0f);
	_camera.SetWindow(width, height);

	effect->GetVariableByName("FarClipDistance")->AsScalar()->SetFloat(_camera.GetFarClip());

	_camera.SetEnablePositionMovement(true);

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

	D3DXMATRIX world_view, view_proj;

	world_view = _world * _view;
	view_proj = _view * _projection;

	Deferred::BoundingFrustum fr(view_proj);

	D3DXVECTOR3 *corners = fr.get_corners();
	D3DXVECTOR4 res;

	// Transform back to view space
	for (int i = 0; i < 4; i++)
	{
		D3DXVec3Transform(&res, &corners[i], &_view);
		corners[i] = D3DXVECTOR3(res.x, res.y, res.z);
	}

	_far_plane_corners_variable->SetFloatVectorArray((float*) corners, 0, 4);

	D3DXMatrixInverse( &_world_view_inv, NULL, &world_view);
	D3DXMatrixTranspose(&_world_view_inv, &_world_view_inv);
	_wv_inverse->SetMatrix((float *)&_world_view_inv);
	_spec_intensity_var->SetFloat(0.8);
}

void Scene::render(ID3D10Device *device, ID3D10EffectPass *pass)
{
	std::vector<Deferred::Object *>::iterator it = _objects.begin();

	while (it != _objects.end())
	{
		Deferred::Object *o = *it;
		bump_shader_variables();

		// Get the object texture
		// TODO: Use materials with properties
		_texture_SR->SetResource(o->get_texture());

		// Apply the shader and draw geometry
		pass->Apply(0);
		o->render();
		++it;
	}
}