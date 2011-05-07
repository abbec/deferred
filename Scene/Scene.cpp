#include "DXUT.h"
#include "DXUTShapes.h"
#include "Scene.h"
#include <cassert>
#include <iostream>

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

	// Create meshes
	DXUTCreateSphere(device, 1.0, 25, 25, &_sphere);
	DXUTCreateTeapot(device, &_teapot);

	const D3D10_INPUT_ELEMENT_DESC *test;
	UINT numTest;
	_sphere->GetVertexDescription(&test, &numTest);

	// Create the input layout
	D3D10_PASS_DESC PassDesc;
    _technique->GetPassByIndex( 0 )->GetDesc( &PassDesc );
    hr = device->CreateInputLayout(test, numTest, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &_layout);

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

    // Initialize the view matrix
    D3DXVECTOR3 Eye( -2.0f, 0.0f, -5.0f );
    D3DXVECTOR3 At( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 Up( 0.0f, 1.0f, 0.0f );
    D3DXMatrixLookAtRH( &_view, &Eye, &At, &Up );

    // Initialize the projection matrix
    D3DXMatrixPerspectiveFovRH( &_projection, ( float )D3DX_PI * 0.5f, width / ( FLOAT )height, 0.1f, 100.0f );

	return S_OK;
}

void Scene::rotate(D3DXVECTOR3 &at)
{
	D3DXVECTOR3 Eye( 0.0f, 0.0f, -5.0f );
    D3DXVECTOR3 Up( 0.0f, 1.0f, 0.0f );
	D3DXMatrixLookAtLH( &_view, &Eye, &at, &Up );
}

void Scene::render(ID3D10Device *device)
{
	device->IASetInputLayout(_layout);

	//
    // Update variables
    //
    _worldVariable->SetMatrix( ( float* )&_world );
    _viewVariable->SetMatrix( ( float* )&_view );
    _projectionVariable->SetMatrix( ( float* )&_projection );

    //
    // Renders a triangle
    //
    D3D10_TECHNIQUE_DESC techDesc;
    _technique->GetDesc( &techDesc );
    for( UINT p = 0; p < techDesc.Passes; ++p )
    {
		D3DXMatrixIdentity( &_world );
		_worldVariable->SetMatrix( ( float* )&_world );
        _technique->GetPassByIndex( p )->Apply( 0 );
		_sphere->DrawSubset(0);

		D3DXMatrixTranslation(&_world, 0.0, 3.0, -1.0);
		_worldVariable->SetMatrix( ( float* )&_world );
		_technique->GetPassByIndex( p )->Apply( 0 );
		_teapot->DrawSubset(0);
    }
}