#include "DXUT.h"
#include "DXUTShapes.h"
#include "Scene.h"
#include <cassert>
#include <iostream>
#include <conio.h>
#include "..\Util\BoundingFrustum.h"

Scene::Scene() :
_worldVariable(NULL), _effect(NULL),sphere(NULL),
_viewVariable(NULL), _projectionVariable(NULL), _ambient_color(0.1, 0.1, 0.1)
{
	
}

Scene::~Scene()
{
	// Clean up objects
	std::vector<Deferred::Object *>::iterator it = _objects.begin();
	while (it != _objects.end())
	{
		Deferred::Object *o = *it;
		delete o;

		++it;
	}

	// Clean up lights
	std::vector<Deferred::Light *>::iterator it2 = _lights.begin();
	while (it2 != _lights.end())
	{
		Deferred::Light *o = *it2;
		delete o;

		++it2;
	}

	SAFE_RELEASE(sphere);
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
	//if (!obj->read_from_obj(device, "Media\\monkey.obj"))
		//_cprintf("Error in initializing OBJ object! \n");

	D3DXMATRIX translate;
	D3DXMatrixTranslation(&translate, -10.5f, 0.0f, 0.0f);
	//obj->set_transform(translate);

	//_objects.push_back(obj);

	// Viking
	//obj = new Deferred::Object();
	if (!obj->read_from_obj(device, "Media\\viking.obj"))
		_cprintf("Error in initializing OBJ object! \n");

	_objects.push_back(obj);

	// Set up lighting
	_lights.push_back(new Deferred::DirectionalLight(D3DXVECTOR4(1.0, 1.0, 1.0, 1.0), 
		D3DXVECTOR3(5.0, 0.0, 0.0), D3DXVECTOR3(0.0, 0.0, 0.0) - D3DXVECTOR3(5.0, 0.0, 0.0)));

	// Initialize the world matrix
    D3DXMatrixIdentity( &_world );

	DXUTCreateSphere(device, 1.0, 100, 100, &sphere);

	// Set up a ModelView Camera
	D3DXVECTOR3 Eye( 0.0f, 0.0f, 2.0f );
    D3DXVECTOR3 At( 0.0f, 0.0f, 0.0f );
	_camera.SetViewParams(&Eye, &At);
	_camera.SetProjParams(( float )D3DX_PI * 0.5f, width / ( float )height, 0.1f, 100.0f);

	_effect->GetVariableByName("FarClipDistance")->AsScalar()->SetFloat(_camera.GetFarClip());
	effect->GetVariableByName("Ambient")->AsVector()->SetFloatVector((float *) _ambient_color);

	_camera.SetEnablePositionMovement(true);
	_camera.SetButtonMasks( MOUSE_LEFT_BUTTON, MOUSE_WHEEL, MOUSE_RIGHT_BUTTON );

	sphere->CommitToDevice();

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
	D3DXMatrixIdentity(&_world);
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
	_effect->GetVariableByName("SpecularRoughness")->AsScalar()->SetFloat(758.8);
	delete[] corners;
}

void Scene::bump_light_variables(Deferred::Light *l)
{
	if (l->get_type() == Deferred::Light::DIRECTIONAL)
	{
		Deferred::DirectionalLight *dl = (Deferred::DirectionalLight *) l;
		_effect->GetVariableByName("LightDir")->AsVector()->SetFloatVector((float*) dl->get_direction());
	}

	// Transform light position to view space
	D3DXMATRIX world_view;
	D3DXMatrixIdentity(&_world); // Do not rotate lights
	world_view = _world * _view;
	D3DXVECTOR4 temp;
	D3DXVECTOR3 lpos((float *) l->get_position());
	D3DXVec3Transform(&temp, &lpos, &_view);
	D3DXVECTOR3 light_pos_vs(temp.x, temp.y, temp.z);

	_effect->GetVariableByName("LightPosition")->AsVector()->SetFloatVector((float *) light_pos_vs);
	_effect->GetVariableByName("LightColor")->AsVector()->SetFloatVector((float *) l->get_color());
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
		//sphere->DrawSubset(0);
		_texture_SR->SetResource(0);
		++it;
	}

	o = NULL;
}

void Scene::draw_lights(ID3D10Device *device)
{
	FLOAT OriginalBlendFactor[4];
    UINT OriginalSampleMask = 0;
    ID3D10BlendState* pOriginalBlendState10=NULL;

	ID3D10DepthStencilView *dsv;
	ID3D10RenderTargetView *rtv;
	device->OMGetRenderTargets(1, &rtv, &dsv);  

	// Save the current blend state   
    device->OMGetBlendState(&pOriginalBlendState10, OriginalBlendFactor, &OriginalSampleMask);   

	D3D10_TECHNIQUE_DESC techDesc;
	
	ID3D10EffectTechnique *tech = _effect->GetTechniqueByName("AmbientLight");
	tech->GetDesc( &techDesc );

    for( UINT p = 0; p < techDesc.Passes; ++p )
    {
		tech->GetPassByIndex(p)->Apply(0);
		device->Draw(4, 0);
	}

	device->ClearDepthStencilView(dsv, D3D10_CLEAR_DEPTH, 1.0, 0);

	// Go through all the lights in the scene
	std::vector<Deferred::Light *>::iterator it = _lights.begin();

	while (it != _lights.end())
	{
		Deferred::Light *light = *it;
		if (light->get_type() == Deferred::Light::DIRECTIONAL)
			tech = _effect->GetTechniqueByName("DirectionalLight");
		else if (light->get_type() == Deferred::Light::POINT)
		{}

		tech->GetDesc( &techDesc );
		bump_light_variables(light);
		for( UINT p = 0; p < techDesc.Passes; ++p )
		{
			tech->GetPassByIndex(p)->Apply(0);
			device->Draw(4, 0);
		}

		++it;
	}

	// Un-set this resource, as it's associated with an OM output
	_effect->GetVariableByName("Normals")->AsShaderResource()->SetResource( NULL );
    _effect->GetVariableByName("Depth")->AsShaderResource()->SetResource( NULL );
	_effect->GetVariableByName("Albedo")->AsShaderResource()->SetResource( NULL );
	_effect->GetVariableByName("SpecularInfo")->AsShaderResource()->SetResource(NULL);

	for( UINT p = 0; p < techDesc.Passes; ++p )
        tech->GetPassByIndex( p )->Apply( 0 );

	// Restore the previous blend state   
	device->OMSetBlendState(pOriginalBlendState10, OriginalBlendFactor, OriginalSampleMask);

	SAFE_RELEASE(pOriginalBlendState10);
	SAFE_RELEASE(rtv);
	SAFE_RELEASE(dsv);
}