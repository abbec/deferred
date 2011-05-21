#include "DXUT.h"
#include "DXUTShapes.h"
#include "Scene.h"
#include <cassert>
#include <iostream>
#include <conio.h>
#include "..\Util\BoundingFrustum.h"

Scene::Scene() :
_worldVariable(NULL), _effect(NULL),
_viewVariable(NULL), _projectionVariable(NULL), _ambient_color(0.1, 0.1, 0.1)
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

	std::vector<Deferred::Light *>::iterator it2 = _lights.begin();

	while (it2 != _lights.end())
	{
		Deferred::Light *o = *it2;
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

	_effect = effect;

    // Obtain the variables
    _worldVariable = _effect->GetVariableByName( "World" )->AsMatrix();
    _viewVariable = _effect->GetVariableByName( "View" )->AsMatrix();
    _projectionVariable = _effect->GetVariableByName( "Projection" )->AsMatrix();
	_wv_inverse = _effect->GetVariableByName( "WorldViewInverse" )->AsMatrix();
	_spec_intensity_var = _effect->GetVariableByName( "SpecularIntensity" )->AsScalar();
	_texture_SR = _effect->GetVariableByName("AlbedoTexture")->AsShaderResource();
	_far_plane_corners_variable = _effect->GetVariableByName("FarPlaneCorners")->AsVector();

	// Objects
	Deferred::Object *obj = new Deferred::Object();
	if (!obj->read_from_obj(device, "Media\\monkey.obj"))
		_cprintf("Error in initializing OBJ object! \n");

	D3DXMATRIX translate;
	D3DXMatrixTranslation(&translate, -10.5f, 0.0f, 0.0f);
	//obj->set_transform(translate);

	_objects.push_back(obj);

	// Viking
	//obj = new Deferred::Object();
	//if (!obj->read_from_obj(device, "Media\\viking.obj"))
		//_cprintf("Error in initializing OBJ object! \n");

	//_objects.push_back(obj);

	// Set up lighting
	//_lights.push_back(new Deferred::DirectionalLight(D3DXVECTOR4(1.0, 1.0, 1.0, 1.0), 
//		D3DXVECTOR3(2.0, 1.0, 2.0), (D3DXVECTOR3(2.0, 1.0, 2.0) - D3DXVECTOR3(0.0, 0.0, 0.0))));

	// Initialize the world matrix
    D3DXMatrixIdentity( &_world );

	// Set up a ModelView Camera
	D3DXVECTOR3 Eye( 0.0f, 0.0f, 2.0f );
    D3DXVECTOR3 At( 0.0f, 0.0f, 0.0f );
	_camera.SetViewParams(&Eye, &At);
	_camera.SetProjParams(( float )D3DX_PI * 0.5f, width / ( float )height, 0.1f, 100.0f);

	_effect->GetVariableByName("FarClipDistance")->AsScalar()->SetFloat(_camera.GetFarClip());
	effect->GetVariableByName("Ambient")->AsVector()->SetFloatVector((float *) _ambient_color);

	_camera.SetEnablePositionMovement(true);

	return S_OK;
}

void Scene::update(double fTime, float fElapsedTime, void* pUserContext)
{
	_camera.FrameMove(fElapsedTime);
}

HRESULT Scene::set_view(const DXGI_SURFACE_DESC *back_buffer_desc)
{
	_camera.SetWindow(back_buffer_desc->Width, back_buffer_desc->Height);
	return S_OK;
}

LRESULT Scene::handle_messages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return _camera.HandleMessages(hWnd, uMsg, wParam, lParam);
}

void Scene::bump_shader_variables(const D3DXMATRIX *translation)
{
    // Update variables
	_world = *_camera.GetWorldMatrix();
	D3DXMatrixMultiply(&_world, &_world, translation);
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

	delete[] corners;
}

void Scene::render(ID3D10Device *device, ID3D10EffectPass *pass)
{
	std::vector<Deferred::Object *>::iterator it = _objects.begin();
	Deferred::Object *o = NULL;

	while (it != _objects.end())
	{
		o = *it;
		bump_shader_variables(o->get_transform());

		// Get the object texture
		// TODO: Use materials with properties
		_texture_SR->SetResource(o->get_texture());

		// Apply the shader and draw geometry
		pass->Apply(0);
		o->render();
		++it;
	}

	o = NULL;
}

void Scene::draw_lights(ID3D10Device *device)
{
	D3D10_TECHNIQUE_DESC techDesc;
	
	ID3D10EffectTechnique *tech = _effect->GetTechniqueByName("AmbientLight");
	tech->GetDesc( &techDesc );

    for( UINT p = 0; p < techDesc.Passes; ++p )
    {
		tech->GetPassByIndex(p)->Apply(0);
		device->Draw(4, 0);
	}

	// Go through all the lights in the scene
	/*std::vector<Deferred::Light *>::iterator it = _lights.begin();

	while (it != _lights.end())
	{
		Deferred::Light *light = *it;
		if (light->get_type() == Deferred::Light::DIRECTIONAL)
		{

		}

		++it;
	}*/
}