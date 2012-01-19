#include "DXUT.h"
#include "DXUTShapes.h"
#include "Scene.h"
#include <cassert>
#include <iostream>
#include <conio.h>
#include "..\Util\BoundingFrustum.h"

Scene::Scene() :
_worldVariable(NULL), _effect(NULL),_skybox(NULL), _skybox_texture_RV(NULL),
_viewVariable(NULL), _projectionVariable(NULL), _ambient_color(0.1, 0.1, 0.1),
_rs_state(NULL), _rs_default_state(NULL), _device(NULL)
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

	SAFE_DELETE(_skybox);
	SAFE_RELEASE(_skybox_texture_RV);
	SAFE_RELEASE(_rs_state);
	SAFE_RELEASE(_rs_default_state);
}

HRESULT Scene::init(ID3D10Device *device, ID3D10Effect *effect)
{
	RECT rc;
    GetClientRect( DXUTGetHWND(), &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

	_device = device;
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
	if (!obj->read_from_obj(device, "Media\\sponza.obj"))
		_cprintf("Error in initializing OBJ object! \n");

	D3DXMATRIX translate;
	D3DXMatrixTranslation(&translate, 0.0f, 50.0f, 0.0f);

	D3DXMATRIX translate2;
	D3DXMatrixTranslation(&translate2, 0.0f, 3.4f, -2.0f);
	D3DXMATRIX scale_viking;
	D3DXMatrixScaling(&scale_viking, 0.2f, 0.2f, 0.2f);
	translate2 = translate2 * scale_viking;

	//obj->set_transform(&skybox);

	_objects.push_back(obj);

	// Viking
	obj = new Deferred::Object();
	if (!obj->read_from_obj(device, "Media\\viking.obj"))
	{
		_cprintf("Error in initializing OBJ object! \n");
		return E_FAIL;
	}

	obj->set_transform(&translate2);
	_objects.push_back(obj);

	D3DXMATRIX skybox;
	D3DXMatrixScaling(&skybox, 50.0, 50.0, 50.0);
	skybox = skybox * translate;

	_skybox = new Deferred::Object();
	_skybox->read_from_obj(_device, "Media\\skysphere.obj");
	_skybox->set_transform(&skybox);

	Deferred::Material *sb_material = new Deferred::Material();
	sb_material->set_diffuse_color(D3DXVECTOR3(1.0, 1.0, 1.0));
	sb_material->create_texture(_device, L"Media\\Textures\\sky.jpg");

	_skybox->set_single_material(sb_material);

	// Set up lighting
	_lights.push_back(new Deferred::DirectionalLight(D3DXVECTOR4(1.0, 0.92, 0.73, 1.0), 
		D3DXVECTOR3(1.0, 1.0, 5.0), D3DXVECTOR3(0.0, 0.0, 0.0) - D3DXVECTOR3(1.0, 1.0, 5.0)));

	_lights.push_back(new Deferred::PointLight(D3DXVECTOR4(1.0, 1.0, 1.0, 1.0), 
		D3DXVECTOR3(-5.0, 0.0, 0.0)));

	_lights.push_back(new Deferred::PointLight(D3DXVECTOR4(1.0, 1.0, 1.0, 1.0), 
		D3DXVECTOR3(5.0, 0.0, 0.0)));

	// Initialize the world matrix
    D3DXMatrixIdentity( &_world );

	// Set up a ModelView Camera
	D3DXVECTOR3 Eye( 8.0f, 8.0f, 0.0f );
    D3DXVECTOR3 At( 0.0f, 0.0f, 0.0f );
	_camera.SetViewParams(&Eye, &At);
	_camera.SetProjParams(( float )D3DX_PI * 0.5f, width / ( float )height, 0.1f, 100.0f);

	_effect->GetVariableByName("FarClipDistance")->AsScalar()->SetFloat(_camera.GetFarClip());
	effect->GetVariableByName("Ambient")->AsVector()->SetFloatVector((float *) _ambient_color);

	_camera.SetEnablePositionMovement(true);
//	_camera.SetButtonMasks( MOUSE_RIGHT_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON );

	return S_OK;
}

void Scene::update(double fTime, float fElapsedTime, void* pUserContext)
{
	_camera.FrameMove(fElapsedTime);
}

const D3DXVECTOR3 *Scene::camera_at()
{
	return _camera.GetEyePt();
}

HRESULT Scene::on_resize(const DXGI_SURFACE_DESC *back_buffer_desc)
{
//	_camera.SetWindow(back_buffer_desc->Width, back_buffer_desc->Height);

	_device->RSGetState(&_rs_default_state);

	D3D10_RASTERIZER_DESC rs_desc;
	_rs_default_state->GetDesc(&rs_desc);
	rs_desc.CullMode = D3D10_CULL_NONE;
	rs_desc.FillMode = D3D10_FILL_SOLID;

	_device->CreateRasterizerState(&rs_desc, &_rs_state);

	return S_OK;
}

void Scene::on_resize_release()
{
	SAFE_RELEASE(_rs_state);
	SAFE_RELEASE(_rs_default_state);
}

LRESULT Scene::handle_messages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return _camera.HandleMessages(hWnd, uMsg, wParam, lParam);
}

void Scene::bump_shader_variables(const Deferred::Object *o, const Deferred::Material *m)
{

	// Update variables
	D3DXMatrixIdentity(&_world);
	_view = *_camera.GetViewMatrix();
	_projection = *_camera.GetProjMatrix();

	D3DXMATRIX world_view;
	D3DXMatrixMultiply(&_world, &_world, o->get_transform());
	
	world_view = _world * _view;

	D3DXMatrixInverse( &_world_view_inv, NULL, &world_view);
	D3DXMatrixTranspose(&_world_view_inv, &_world_view_inv);

    _worldVariable->SetMatrix( ( float* )&_world );
    _viewVariable->SetMatrix( ( float* )&_view );
    _projectionVariable->SetMatrix( ( float* )&_projection );

	D3DXMATRIX view_proj;

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

	_wv_inverse->SetMatrix((float *)&_world_view_inv);


	_spec_intensity_var->SetFloat(m->get_specular_intensity());
	_effect->GetVariableByName("SpecularPower")->AsScalar()->SetFloat(m->get_specular_power());
	_effect->GetVariableByName("Alpha")->AsScalar()->SetFloat(m->get_alpha());
	_effect->GetVariableByName("DiffuseColor")->AsVector()->SetFloatVector((float *) m->get_diffuse_color());

	_texture_SR->SetResource(m->get_texture());


	delete[] corners;
}

void Scene::bump_light_variables(Deferred::Light *l)
{
	// Transform light position to view space
	D3DXMATRIX world_view;
	D3DXMatrixIdentity(&_world); // Do not rotate lights
	world_view = _world * _view;
	D3DXVECTOR4 temp;
	D3DXVECTOR3 lpos((float *) l->get_position());
	D3DXVec3Transform(&temp, &lpos, &_view);
	D3DXVECTOR3 light_pos_vs(temp.x, temp.y, temp.z);

	if (l->get_type() == Deferred::Light::DIRECTIONAL)
	{
		Deferred::DirectionalLight *dl = (Deferred::DirectionalLight *) l;

		D3DXVECTOR3 ldir((float *) dl->get_direction());
		D3DXVec3Transform(&temp, &ldir, &_view);
		D3DXVECTOR3 light_dir_vs(temp.x, temp.y, temp.z);
		_effect->GetVariableByName("LightDir")->AsVector()->SetFloatVector((float*) light_dir_vs);
	}

	_effect->GetVariableByName("LightPosition")->AsVector()->SetFloatVector((float *) light_pos_vs);
	_effect->GetVariableByName("LightColor")->AsVector()->SetFloatVector((float *) l->get_color());
}

void Scene::render_skybox(ID3D10Device *device)
{
	//_skybox->render();
}

void Scene::render(ID3D10Device *device, ID3D10Effect *effect)
{

	ID3D10EffectTechnique *tech = _effect->GetTechniqueByName("GeometryStageNoSpecular");
	D3D10_TECHNIQUE_DESC techDesc;

	tech->GetDesc(&techDesc);

	/*_device->RSSetState(_rs_state);
	_texture_SR->SetResource(_skybox_texture_RV);
	bump_shader_variables(_skybox, _skybox->get_subset_material(0));

	for( UINT p = 0; p < techDesc.Passes; ++p )
	{
		tech->GetPassByIndex(p)->Apply(0);
		_skybox->render();
	}

	_device->RSSetState(_rs_default_state);*/


	// Render all objects
	std::vector<Deferred::Object *>::iterator it = _objects.begin();
	Deferred::Object *o = NULL;
	std::vector<UINT> transparent;
	std::vector<UINT>::iterator it2;
	UINT num_subsets;
	const Deferred::Material *m = NULL;

	while (it != _objects.end())
	{
		o = *it;

		num_subsets = o->get_num_subsets();

		for (UINT i = 0; i < num_subsets; i++)
		{
			m = o->get_subset_material(i);

			if (m->get_alpha() < 1.0)
			{
				transparent.push_back(i);
			}
			else
			{
				tech = _effect->GetTechniqueByName(m->get_technique().c_str());
				tech->GetDesc(&techDesc);

				bump_shader_variables(o, m);

				for( UINT p = 0; p < techDesc.Passes; ++p )
				{
					tech->GetPassByIndex(p)->Apply(0);
					o->render_subset(i);
			
					_texture_SR->SetResource(0);
					tech->GetPassByIndex(p)->Apply(0);
				}
			}
		}

		// Render transparent subsets last
		it2 = transparent.begin();
		while (it2 != transparent.end())
		{
			m = o->get_subset_material(*it2);
			std::string tech_name(m->get_technique());
			tech_name.append("Alpha");
			tech = _effect->GetTechniqueByName(tech_name.c_str());
			tech->GetDesc(&techDesc);

			bump_shader_variables(o, m);

			for( UINT p = 0; p < techDesc.Passes; ++p )
			{
				tech->GetPassByIndex(p)->Apply(0);
				o->render_subset(*it2);

				_texture_SR->SetResource(0);
				tech->GetPassByIndex(p)->Apply(0);
			}

			++it2;
		}
	
		transparent.clear();
		++it;
	}

	o = NULL;
}

void Scene::draw_lights(ID3D10Device *device)
{
	ID3D10DepthStencilView *dsv;
	ID3D10RenderTargetView *rtv;
	device->OMGetRenderTargets(1, &rtv, &dsv);

	D3D10_TECHNIQUE_DESC techDesc;
	
	ID3D10EffectTechnique *tech = _effect->GetTechniqueByName("AmbientLight");
	tech->GetDesc( &techDesc );

    for( UINT p = 0; p < techDesc.Passes; ++p )
    {
		tech->GetPassByIndex(p)->Apply(0);
		device->Draw(4, 0);
	}

	// Go through all the lights in the scene
	std::vector<Deferred::Light *>::iterator it = _lights.begin();

	while (it != _lights.end())
	{
		Deferred::Light *light = *it;
		if (light->get_type() == Deferred::Light::DIRECTIONAL)
			tech = _effect->GetTechniqueByName("DirectionalLight");
		else if (light->get_type() == Deferred::Light::POINT)
			tech = _effect->GetTechniqueByName("PointLight");

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

	for( UINT p = 0; p < techDesc.Passes; ++p )
        tech->GetPassByIndex( p )->Apply( 0 );

	SAFE_RELEASE(rtv);
	SAFE_RELEASE(dsv);
}