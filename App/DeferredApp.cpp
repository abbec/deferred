#include "DXUT.h"
//#include "../Scene/Scene.h"
#include "DeferredApp.h"
#include <iostream>
#include <conio.h>

DeferredApp *DeferredApp::inst = NULL;

DeferredApp::DeferredApp() :
_device(NULL), _time(0.0), 
_elapsed_time(0.0), _user_context(NULL),
_scene(), _layout(NULL), _effect(NULL), _geometry_stage(NULL),
_g_normals(NULL), _g_depth(NULL), _quad_layout(NULL), _quad_VB(NULL)
{
	_g_buffer_views[0] = _g_buffer_views[1] = NULL;
	_g_buffer_SRV[0] = _g_buffer_SRV[1] = NULL;
}

DeferredApp::~DeferredApp()
{
	if (_layout != NULL)
		_layout->Release();

	if (_effect != NULL)
		_effect->Release();

	SAFE_RELEASE(_g_normals);

	SAFE_RELEASE(_g_depth);

	SAFE_RELEASE(_g_buffer_views[0]);
	SAFE_RELEASE(_g_buffer_views[1]);

	SAFE_RELEASE(_g_buffer_SRV[0]);
	SAFE_RELEASE(_g_buffer_SRV[1]);

	SAFE_RELEASE(_quad_VB);
	SAFE_RELEASE(_quad_layout);

	inst = NULL;
}

DeferredApp *DeferredApp::instance()
{
	if (!inst)
		inst = new DeferredApp();

	return inst;
}

HRESULT DeferredApp::initScene(ID3D10Device *device)
{
	// Save device for later use
	_device = device;

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
	hr = D3DX10CreateEffectFromFile( L"Shaders\\Deferred.fx", NULL, NULL, "fx_4_0", dwShaderFlags, 0, device, NULL,
                                         NULL, &_effect, &ppErrors, NULL );

    if( FAILED( hr ) )
    {
      	if (ppErrors)
			_cprintf("Shader compile error: %s\n", (char*)ppErrors->GetBufferPointer());

		return hr;
    }

	 // Obtain the technique
    _geometry_stage = _effect->GetTechniqueByName( "GeometryStage" );
	// Create the input layout
	D3D10_PASS_DESC PassDesc;
    _geometry_stage->GetPassByIndex( 0 )->GetDesc( &PassDesc );
	hr = device->CreateInputLayout(Deferred::Object::LAYOUT, Deferred::Object::NUM_LAYOUT_ELMS, 
		PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &_layout);

	if (FAILED(hr))
	{
		_cprintf("Creation of input layout failed!");
		return hr;
	}

	const D3D10_INPUT_ELEMENT_DESC screenlayout[] =
    {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D10_INPUT_PER_VERTEX_DATA, 0 },
    };

	_render_to_quad = _effect->GetTechniqueByName( "RenderToQuad" );
	_render_to_quad->GetPassByIndex( 0 )->GetDesc( &PassDesc );
	hr = _device->CreateInputLayout( screenlayout, 1, PassDesc.pIAInputSignature,
                                             PassDesc.IAInputSignatureSize, &_quad_layout);

	if (FAILED(hr))
	{
		_cprintf("Creation of input layout failed!");
		return hr;
	}


	//_g_buffer_SR = _effect->GetVariableByName("g_buffer");

	device->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	// Create the screen quad
	D3D10_BUFFER_DESC BDesc;
    BDesc.ByteWidth = 4 * sizeof( D3DXVECTOR3 );
    BDesc.Usage = D3D10_USAGE_IMMUTABLE;
    BDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    BDesc.CPUAccessFlags = 0;
    BDesc.MiscFlags = 0;

    D3DXVECTOR3 verts[4] =
    {
        D3DXVECTOR3( -1, -1, 0.5f ),
        D3DXVECTOR3( -1, 1, 0.5f ),
        D3DXVECTOR3( 1, -1, 0.5f ),
        D3DXVECTOR3( 1, 1, 0.5f )
    };
    D3D10_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = verts;
    V_RETURN( _device->CreateBuffer( &BDesc, &InitData, &_quad_VB ) );


	return _scene.init(device, _effect);
}

bool DeferredApp::initBuffers(ID3D10Device *device, const DXGI_SURFACE_DESC *back_buffer_desc)
{
	// Set up MRT
	D3D10_TEXTURE2D_DESC Desc;
    Desc.Width = back_buffer_desc->Width;
    Desc.Height = back_buffer_desc->Height;
    Desc.MipLevels = 1;
    Desc.ArraySize = 1;
    Desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    Desc.SampleDesc.Count = 1;
    Desc.SampleDesc.Quality = 0;
    Desc.Usage = D3D10_USAGE_DEFAULT;
    Desc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
    Desc.CPUAccessFlags = 0;
    Desc.MiscFlags = 0;

	device->CreateTexture2D(&Desc, NULL, &_g_normals);
	device->CreateTexture2D(&Desc, NULL, &_g_depth);

	D3D10_RENDER_TARGET_VIEW_DESC RTVDesc;
    RTVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    RTVDesc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
    RTVDesc.Texture2D.MipSlice = 0;

	if FAILED(device->CreateRenderTargetView(_g_normals, &RTVDesc, &_g_buffer_views[0]))
	{
		_cprintf("Creation of render target view failed.");
		return false;
	}

	if FAILED(device->CreateRenderTargetView(_g_depth, &RTVDesc, &_g_buffer_views[1]))
	{
		_cprintf("Creation of render target view failed.");
		return false;
	}

	// Shader resource views
	D3D10_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    SRVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    SRVDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Texture2D.MostDetailedMip = 0;
    SRVDesc.Texture2D.MipLevels = Desc.MipLevels;

    device->CreateShaderResourceView(_g_normals, &SRVDesc, &_g_buffer_SRV[0] );
    device->CreateShaderResourceView(_g_depth, &SRVDesc, &_g_buffer_SRV[1] );

	return true;
}

void DeferredApp::update(double fTime, float fElapsedTime, void* pUserContext)
{
	_scene.update(fTime, fElapsedTime, pUserContext);
}

LRESULT DeferredApp::handle_messages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	// Pass user messages on to the scene
	_scene.handle_messages(hWnd, uMsg, wParam, lParam);

	return 0;
}

void DeferredApp::render(ID3D10Device* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext)
{
	_time = fTime;
	_elapsed_time = fElapsedTime;
	_user_context = pUserContext;

	// Clear render target and the depth stencil 
    float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    pd3dDevice->ClearRenderTargetView( DXUTGetD3D10RenderTargetView(), ClearColor );
    pd3dDevice->ClearDepthStencilView( DXUTGetD3D10DepthStencilView(), D3D10_CLEAR_DEPTH, 1.0, 0 );

	// Fill G-buffers
	geometry_stage();


	// P-buffers


	//Final composition
	render_to_quad();

}

void DeferredApp::render_to_quad()
{
	// Render the particles
    ID3D10EffectTechnique* pRenderTechnique = _render_to_quad;

    //IA setup
    _device->IASetInputLayout( _quad_layout );
    UINT Strides[1];
    UINT Offsets[1];
    ID3D10Buffer* pVB[1];
    pVB[0] = _quad_VB;
    Strides[0] = sizeof( D3DXVECTOR3 );
    Offsets[0] = 0;
    _device->IASetVertexBuffers( 0, 1, pVB, Strides, Offsets );
    _device->IASetIndexBuffer( NULL, DXGI_FORMAT_R16_UINT, 0 );
    _device->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

	_effect->GetVariableByName("Normals")->AsShaderResource()->SetResource( _g_buffer_SRV[0] );
    _effect->GetVariableByName("Depth")->AsShaderResource()->SetResource( _g_buffer_SRV[1] );

    //Render
    D3D10_TECHNIQUE_DESC techDesc;
    pRenderTechnique->GetDesc( &techDesc );

    for( UINT p = 0; p < techDesc.Passes; ++p )
    {
        pRenderTechnique->GetPassByIndex( p )->Apply( 0 );
        _device->Draw( 4, 0 );
    }

	// Un-set this resource, as it's associated with an OM output
	_effect->GetVariableByName("Normals")->AsShaderResource()->SetResource( NULL );
    _effect->GetVariableByName("Depth")->AsShaderResource()->SetResource( NULL );
    for( UINT p = 0; p < techDesc.Passes; ++p )
    {
        pRenderTechnique->GetPassByIndex( p )->Apply( 0 );
    }
}

void DeferredApp::geometry_stage()
{
	// Clear the new render target
    float color[4] =
    {
        0, 0, 0, 0
    };
    _device->ClearRenderTargetView( _g_buffer_views[0], color );
    _device->ClearRenderTargetView( _g_buffer_views[1], color );

	// get the old render targets
    ID3D10RenderTargetView* pOldRTV;
    ID3D10DepthStencilView* pOldDSV;
    _device->OMGetRenderTargets( 1, &pOldRTV, &pOldDSV );

	// Set the new render targets
    ID3D10RenderTargetView *views[2];
	views[0] = _g_buffer_views[0];
    views[1] = _g_buffer_views[1];
    _device->OMSetRenderTargets( 2, views, pOldDSV);

	_device->IASetInputLayout(_layout);

	_scene.bump_shader_variables();


    D3D10_TECHNIQUE_DESC techDesc;
	
    _geometry_stage->GetDesc( &techDesc );
    for( UINT p = 0; p < techDesc.Passes; ++p )
    {
		_geometry_stage->GetPassByIndex( p )->Apply( 0 );
		_scene.render(_device);
	}

	// restore the original render targets
    views[0] = pOldRTV;
    views[1] = NULL;
    _device->OMSetRenderTargets( 2, views, pOldDSV );
    SAFE_RELEASE( pOldRTV );
    SAFE_RELEASE( pOldDSV );
}