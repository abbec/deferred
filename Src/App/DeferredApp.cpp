#include "DXUT.h"
#include "DeferredApp.h"
#include <iostream>
#include <conio.h>

DeferredApp *DeferredApp::inst = NULL;

DeferredApp::DeferredApp() :
_device(NULL), _time(0.0), 
_elapsed_time(0.0), _user_context(NULL), _backbuffer(NULL), _depth_stencil(NULL),
_scene(), _layout(NULL), _effect(NULL), _geometry_stage(NULL),
_quad_layout(NULL), _quad_VB(NULL), _render_state(FINAL), _hud(NULL), _deferred(true)
{
	for (int i = 0; i < GBUFFER_SIZE; ++i)
	{
		_g_textures[i] = NULL;
		_g_buffer_views[i] = NULL;
		_g_buffer_SRV[i] = NULL;
	}

	for (int i = 0; i < PBUFFER_SIZE; ++i)
	{
		_p_textures[i] = NULL;
		_p_buffer_views[i] = NULL;
		_p_buffer_SRV[i] = NULL;
	}
}

DeferredApp::~DeferredApp()
{
	SAFE_RELEASE(_layout);

	SAFE_RELEASE(_effect);

	for (int i = 0; i < GBUFFER_SIZE; ++i)
	{
		SAFE_RELEASE(_g_textures[i]);
		SAFE_RELEASE(_g_buffer_views[i]);
		SAFE_RELEASE(_g_buffer_SRV[i]);
	}

	for (int i = 0; i < PBUFFER_SIZE; ++i)
	{
		SAFE_RELEASE(_p_textures[i]);
		SAFE_RELEASE(_p_buffer_views[i]);
		SAFE_RELEASE(_p_buffer_SRV[i]);
	}

	SAFE_RELEASE(_quad_VB);
	SAFE_RELEASE(_quad_layout);
	SAFE_RELEASE(_backbuffer);
	SAFE_RELEASE(_depth_stencil);

	SAFE_DELETE(_hud);

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
	hr = D3DX10CreateEffectFromFileW( L"Shaders\\Deferred.fx", NULL, NULL, "fx_4_0", dwShaderFlags, 0, device, NULL,
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
		{ "TEXCOORD",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,  D3D10_INPUT_PER_VERTEX_DATA, 0 },
    };

	_render_to_quad = _effect->GetTechniqueByName( "RenderToQuad" );
	_render_normals_to_quad = _effect->GetTechniqueByName( "RenderNormalsToQuad" );
	_render_depth_to_quad = _effect->GetTechniqueByName( "RenderDepthToQuad" );
	_render_albedo_to_quad = _effect->GetTechniqueByName( "RenderAlbedoToQuad" );
	_render_to_quad->GetPassByIndex( 0 )->GetDesc( &PassDesc );
	hr = _device->CreateInputLayout( screenlayout, 2, PassDesc.pIAInputSignature,
                                             PassDesc.IAInputSignatureSize, &_quad_layout);

	if (FAILED(hr))
	{
		_cprintf("Creation of input layout failed!");
		return hr;
	}

	device->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	// Create the screen quad
	D3D10_BUFFER_DESC BDesc;
    BDesc.ByteWidth = 4 * sizeof( QuadVertex );
    BDesc.Usage = D3D10_USAGE_IMMUTABLE;
    BDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    BDesc.CPUAccessFlags = 0;
    BDesc.MiscFlags = 0;

    QuadVertex verts[] =
    {
		{ D3DXVECTOR3( -1.0, -1.0, 0.5f ), D3DXVECTOR3(0.0, 1.0, 1.0)},
		{ D3DXVECTOR3( -1.0, 1.0, 0.5f ), D3DXVECTOR3(0.0, 0.0, 0.0)},
		{ D3DXVECTOR3( 1.0, -1.0, 0.5f ), D3DXVECTOR3(1.0, 1.0, 2.0)},
        { D3DXVECTOR3( 1.0, 1.0, 0.5f ), D3DXVECTOR3(1.0, 0.0, 3.0)},
    };
    D3D10_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = verts;
    _device->CreateBuffer(&BDesc, &InitData, &_quad_VB);

	// Create HUD
	_hud = new Deferred::Hud(_device);

	// Init scene
	return _scene.init(device, _effect);
}

void DeferredApp::clean_buffers()
{
	// Make sure the old buffers are released (Mostly on resize)
	for (int i = 0; i < GBUFFER_SIZE; ++i)
	{
		SAFE_RELEASE(_g_textures[i]);
		SAFE_RELEASE(_g_buffer_views[i]);
		SAFE_RELEASE(_g_buffer_SRV[i]);
	}

	for (int i = 0; i < PBUFFER_SIZE; ++i)
	{
		SAFE_RELEASE(_p_textures[i]);
		SAFE_RELEASE(_p_buffer_views[i]);
		SAFE_RELEASE(_p_buffer_SRV[i]);
	}

	_scene.on_resize_release();
}

HRESULT DeferredApp::initBuffers(ID3D10Device *device, const DXGI_SURFACE_DESC *back_buffer_desc)
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

	D3D10_RENDER_TARGET_VIEW_DESC RTVDesc;
    RTVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    RTVDesc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
    RTVDesc.Texture2D.MipSlice = 0; // Use mip slice 0

	// Shader resource views
	D3D10_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    SRVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    SRVDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Texture2D.MostDetailedMip = 0;
    SRVDesc.Texture2D.MipLevels = Desc.MipLevels;

	for (int i = 0; i < GBUFFER_SIZE; ++i)
	{

		if FAILED(device->CreateTexture2D(&Desc, NULL, &_g_textures[i]))
		{
			_cprintf("Creation of texture failed.");
			return false;
		}

		if FAILED(device->CreateRenderTargetView(_g_textures[i], &RTVDesc, &_g_buffer_views[i]))
		{
			_cprintf("Creation of render target view failed.");
			return false;
		}

		if FAILED(device->CreateShaderResourceView(_g_textures[i], &SRVDesc, &_g_buffer_SRV[i] ))
		{
			_cprintf("Creation of shader resource view failed.");
			return false;
		}
		
	}

	for (int i = 0; i < PBUFFER_SIZE; ++i)
	{
		if FAILED(device->CreateTexture2D(&Desc, NULL, &_p_textures[i]))
		{
			_cprintf("Creation of texture failed.");
			return false;
		}

		if FAILED(device->CreateRenderTargetView(_p_textures[i], &RTVDesc, &_p_buffer_views[i]))
		{
			_cprintf("Creation of render target view failed.");
			return false;
		}

		if FAILED(device->CreateShaderResourceView(_p_textures[i], &SRVDesc, &_p_buffer_SRV[i] ))
		{
			_cprintf("Creation of shader resource view failed.");
			return false;
		}	
	}

	return _scene.on_resize(back_buffer_desc);
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

    float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	// Get the old render targets
    _device->OMGetRenderTargets( 1, &_backbuffer, &_depth_stencil );
    
	// Clear framebuffer and depth stencil
	_device->ClearRenderTargetView(_backbuffer, ClearColor);
    _device->ClearDepthStencilView(_depth_stencil, D3D10_CLEAR_DEPTH, 1.0, 0);

	if (_deferred)
	{
		// Clear G-buffers
		for (int i = 0; i < GBUFFER_SIZE; ++i)
			_device->ClearRenderTargetView( _g_buffer_views[i], ClearColor);

		for (int i = 0; i < PBUFFER_SIZE; ++i)
			_device->ClearRenderTargetView( _p_buffer_views[i], ClearColor);

		// Fill G-buffers
		geometry_stage();

		// Lighting stage
		lighting_stage();

		// Reset backbuffer as render target
		_device->OMSetRenderTargets( 1, &_backbuffer, _depth_stencil );
	
		// Final composition
		render_to_quad();
	}
	else
	{

		ID3D10EffectTechnique *temp = _effect->GetTechniqueByName("GBufferToScreen");
		_device->IASetInputLayout(_layout);

		D3D10_TECHNIQUE_DESC techDesc;
	
		temp->GetDesc( &techDesc );
	
		// Render scene
		for( UINT p = 0; p < techDesc.Passes; ++p )
			_scene.render(_device, temp->GetPassByIndex( p ));
	}

	// Render text
	_hud->render();
	SAFE_RELEASE(_backbuffer);
	SAFE_RELEASE(_depth_stencil);
}

void DeferredApp::render_to_quad()
{
    ID3D10EffectTechnique* pRenderTechnique;

	switch (_render_state)
	{
	case NORMALS:
		pRenderTechnique = _render_normals_to_quad;
		break;
	case DEPTH:
		pRenderTechnique = _render_depth_to_quad;
		break;
	case ALBEDO:
		pRenderTechnique = _render_albedo_to_quad;
		break;
	default:
		pRenderTechnique = _render_to_quad;
	}

    // Input Assembly setup
    _device->IASetInputLayout( _quad_layout );
    UINT Strides[1];
    UINT Offsets[1];
    ID3D10Buffer* pVB[1];
    pVB[0] = _quad_VB;
    Strides[0] = sizeof( QuadVertex );
    Offsets[0] = 0;
    _device->IASetVertexBuffers( 0, 1, pVB, Strides, Offsets );
    _device->IASetIndexBuffer( NULL, DXGI_FORMAT_R16_UINT, 0 );
    _device->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

	_effect->GetVariableByName("Normals")->AsShaderResource()->SetResource( _g_buffer_SRV[0] );
    _effect->GetVariableByName("Depth")->AsShaderResource()->SetResource( _g_buffer_SRV[1] );
	_effect->GetVariableByName("Albedo")->AsShaderResource()->SetResource( _g_buffer_SRV[2] );
	_effect->GetVariableByName("FinalImage")->AsShaderResource()->SetResource( _p_buffer_SRV[0] );

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
	_effect->GetVariableByName("Albedo")->AsShaderResource()->SetResource( NULL );
	_effect->GetVariableByName("FinalImage")->AsShaderResource()->SetResource( NULL );
    for( UINT p = 0; p < techDesc.Passes; ++p )
        pRenderTechnique->GetPassByIndex( p )->Apply( 0 );
}

void DeferredApp::lighting_stage()
{
	// Set the new render targets
    ID3D10RenderTargetView *views[PBUFFER_SIZE];

	for (int i = 0; i < PBUFFER_SIZE; ++i)
		views[i] = _p_buffer_views[i];

    _device->OMSetRenderTargets(PBUFFER_SIZE, views, _depth_stencil);

	//IA setup
    _device->IASetInputLayout( _quad_layout );
    UINT Strides[1];
    UINT Offsets[1];
    ID3D10Buffer* pVB[1];
    pVB[0] = _quad_VB;
    Strides[0] = sizeof( QuadVertex );
    Offsets[0] = 0;
    _device->IASetVertexBuffers( 0, 1, pVB, Strides, Offsets );
    _device->IASetIndexBuffer( NULL, DXGI_FORMAT_R16_UINT, 0 );
    _device->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

	D3DXMATRIX ortho;
	D3DXMatrixOrthoRH(&ortho, 2.0, 2.0, 0.0, 1.0);

	_effect->GetVariableByName("Ortho")->AsMatrix()->SetMatrix((float *) ortho);

	_effect->GetVariableByName("Normals")->AsShaderResource()->SetResource( _g_buffer_SRV[0] );
    _effect->GetVariableByName("Depth")->AsShaderResource()->SetResource( _g_buffer_SRV[1] );
	_effect->GetVariableByName("Albedo")->AsShaderResource()->SetResource( _g_buffer_SRV[2] );
	
	_scene.draw_lights(_device);
}

void DeferredApp::geometry_stage()
{
	// Set the new render targets
    ID3D10RenderTargetView *views[GBUFFER_SIZE];

	for (int i = 0; i < GBUFFER_SIZE; ++i)
		views[i] = _g_buffer_views[i];

    _device->OMSetRenderTargets(GBUFFER_SIZE, views, _depth_stencil);
	_device->IASetInputLayout(_layout);

    D3D10_TECHNIQUE_DESC techDesc;
	
    _geometry_stage->GetDesc( &techDesc );
	
	// Render scene
	for( UINT p = 0; p < techDesc.Passes; ++p )
		_scene.render(_device, _geometry_stage->GetPassByIndex( p ));
}