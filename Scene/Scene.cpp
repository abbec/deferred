#include "DXUT.h"
#include "DXUTShapes.h"
#include "Scene.h"
#include <cassert>
#include <iostream>
#include <conio.h>

Scene::Scene() :
_sphere(NULL), _layout(NULL), _effect(NULL), _worldVariable(NULL),
_viewVariable(NULL), _projectionVariable(NULL), _technique(NULL),
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
	
	if (_layout != NULL)
		_layout->Release();

	if (_effect != NULL)
		_effect->Release();
}

HRESULT Scene::init(ID3D10Device *device)
{
	RECT rc;
    GetClientRect( DXUTGetHWND(), &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

	HRESULT hr;
	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;

#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3D10_SHADER_DEBUG;
#endif

	ID3D10Blob *ppErrors = NULL;
	// Create a shader
	hr = D3DX10CreateEffectFromFile( L"Shaders/Deferred.fx", NULL, NULL, "fx_4_0", dwShaderFlags, 0, device, NULL,
                                         NULL, &_effect, &ppErrors, NULL );
    if( FAILED( hr ) )
    {
      	if (ppErrors)
		{     
			std::string s("Shader compile error: ");
			s += ((char*)ppErrors->GetBufferPointer());
			MessageBoxA(NULL, s.c_str(), "Error", MB_OK);
		}

		return hr;
    }

	 // Obtain the technique
    _technique = _effect->GetTechniqueByName( "Render" );

    // Obtain the variables
    _worldVariable = _effect->GetVariableByName( "World" )->AsMatrix();
    _viewVariable = _effect->GetVariableByName( "View" )->AsMatrix();
    _projectionVariable = _effect->GetVariableByName( "Projection" )->AsMatrix();
	_wv_inverse = _effect->GetVariableByName( "WorldViewInverse" )->AsMatrix();
	_spec_intensity_var = _effect->GetVariableByName( "SpecularIntensity" )->AsScalar();

	// Create meshes
	DXUTCreateSphere(device, 1.0, 25, 25, &_sphere);
	DXUTCreateBox(device, 1.0, 1.0, 1.0, &_teapot);

	if (!_object.read_from_obj(device, "Media\\viking.obj"))
		_cprintf("Error in initializing OBJ object! \n");

	// Create the input layout
	D3D10_PASS_DESC PassDesc;
    _technique->GetPassByIndex( 0 )->GetDesc( &PassDesc );
	hr = device->CreateInputLayout(Deferred::Object::LAYOUT, Deferred::Object::NUM_LAYOUT_ELMS, 
		PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &_layout);

	if (FAILED(hr))
	{
		MessageBoxA(NULL, "Creation of input layout failed!", "Error", MB_OK);

		_sphere->Release();
		_sphere = NULL;
		_teapot->Release();
		_teapot = NULL;

		return hr;
	}

	device->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

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

void Scene::render(ID3D10Device *device)
{
	device->IASetInputLayout(_layout);

	//
    // Update variables
    //
	_world = *_camera.GetWorldMatrix();
	_view = *_camera.GetViewMatrix();
	_projection = *_camera.GetProjMatrix();

    _worldVariable->SetMatrix( ( float* )&_world );
    _viewVariable->SetMatrix( ( float* )&_view );
    _projectionVariable->SetMatrix( ( float* )&_projection );

    //
    // Renders a triangle
    //
    D3D10_TECHNIQUE_DESC techDesc;
	D3DXMATRIX world_view;
    _technique->GetDesc( &techDesc );
    for( UINT p = 0; p < techDesc.Passes; ++p )
    {
		world_view = _world * _view;
		D3DXMatrixInverse( &_world_view_inv, NULL, &world_view);
		D3DXMatrixTranspose(&_world_view_inv, &_world_view_inv);
		_wv_inverse->SetMatrix((float *)&_world_view_inv);
		_worldVariable->SetMatrix( ( float* )&_world );
		_spec_intensity_var->SetFloat(0.8);
		_technique->GetPassByIndex( p )->Apply( 0 );
		//_teapot->DrawSubset(0);
		_object.render();
    }
}