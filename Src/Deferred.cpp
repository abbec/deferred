//--------------------------------------------------------------------------------------
// File: Deferred.cpp
//
// Empty starting point for new Direct3D 9 and/or Direct3D 10 applications
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "App/DeferredApp.h"
#include <conio.h>

DeferredApp *instance;
static UINT num_screenshots = 0;

//--------------------------------------------------------------------------------------
// Reject any D3D10 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D10DeviceAcceptable( UINT Adapter, UINT Output, D3D10_DRIVER_TYPE DeviceType,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
	// Disable VSync
	pDeviceSettings->d3d10.SyncInterval = 0;
    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D10 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D10CreateDevice( ID3D10Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
	return DeferredApp::instance()->initScene(pd3dDevice);
}


//--------------------------------------------------------------------------------------
// Create any D3D10 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D10ResizedSwapChain( ID3D10Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
	return DeferredApp::instance()->initBuffers(pd3dDevice, pBackBufferSurfaceDesc);
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
	// Update the camera
	DeferredApp::instance()->update(fTime, fElapsedTime, pUserContext);
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D10 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10FrameRender( ID3D10Device* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
	DeferredApp::instance()->render(pd3dDevice, fTime, fElapsedTime, pUserContext);
}


//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10ReleasingSwapChain( void* pUserContext )
{
	if (instance)
		DeferredApp::instance()->clean_buffers();
}


//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10DestroyDevice( void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                          bool* pbNoFurtherProcessing, void* pUserContext )
{

	// Pass all remaining windows messages to camera so it can respond to user input
	DeferredApp::instance()->handle_messages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
	if (!bKeyDown)
	{
		D3DXVECTOR3 at;

		switch (nChar)
		{
		case 49: case 97:
			DeferredApp::instance()->set_render_state(DeferredApp::NORMALS);
			_cprintf("Setting render state to NORMALS.\n");
			break;
		case 50: case 98:
			DeferredApp::instance()->set_render_state(DeferredApp::DEPTH);
			_cprintf("Setting render state to DEPTH.\n");
			break;
		case 51: case 99:
			DeferredApp::instance()->set_render_state(DeferredApp::ALBEDO);
			_cprintf("Setting render state to ALBEDO_TEXTURE.\n");
			break;
		case 52: case 100:
			DeferredApp::instance()->set_render_state(DeferredApp::SPECULAR_INTENSITY);
			_cprintf("Setting render state to SPECULAR_INTENSITY.\n");
			break;
		case 53: case 101:
			DeferredApp::instance()->set_render_state(DeferredApp::FINAL);
			_cprintf("Setting render state to FINAL_COMPOSIT.\n");
			break;
		case 66:
			DeferredApp::instance()->forward_rendering();
			_cprintf("Using forward rendering.\n");
			break;
		case 78:
			DeferredApp::instance()->deferred_rendering();
			_cprintf("Using deferred rendering.\n");
			break;
		case 122:
			DXUTToggleFullScreen();
			break;
		case 'C':
			at = *DeferredApp::instance()->getScene()->camera_at();
			_cprintf("Camera is at (%f, %f, %f)\n", at.x, at.y, at.z);
			break;
		case 'P': // Render screenshot		

			ID3D10Resource *backbufferRes;
			DXUTGetD3D10RenderTargetView()->GetResource(&backbufferRes);

			D3D10_TEXTURE2D_DESC texDesc;
			texDesc.ArraySize = 1;
			texDesc.BindFlags = 0;
			texDesc.CPUAccessFlags = 0;
			texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			texDesc.Width = DXUTGetDXGIBackBufferSurfaceDesc()->Width;
			texDesc.Height = DXUTGetDXGIBackBufferSurfaceDesc()->Height; 
			texDesc.MipLevels = 1;
			texDesc.MiscFlags = 0;
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.Usage = D3D10_USAGE_DEFAULT;

			ID3D10Texture2D *tex;
			DXUTGetD3D10Device()->CreateTexture2D(&texDesc, 0, &tex);
			DXUTGetD3D10Device()->CopyResource(tex, backbufferRes);

			// Check that the directory exists and how many screenshots there
			// are already
			DWORD dwAttrib = GetFileAttributes(L"Screenshots/");

			if (dwAttrib == INVALID_FILE_ATTRIBUTES || dwAttrib & FILE_ATTRIBUTE_DIRECTORY)
				CreateDirectory(L"Screenshots", NULL);

			WCHAR buff[256];
			wsprintf(buff, L"Screenshots/screenshot_%u.png", num_screenshots);
			dwAttrib = GetFileAttributes(buff);
			while (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
			{
				++num_screenshots;
				wsprintf(buff, L"Screenshots/screenshot_%u.png", num_screenshots);
				dwAttrib = GetFileAttributes(buff);
			}


			D3DX10SaveTextureToFile(tex, D3DX10_IFF_PNG, buff);
			_cwprintf(L"Saved screenshot to: %s\n", buff);
			++num_screenshots;

			tex->Release();
			backbufferRes->Release();
			break;
		}

	}
}


//--------------------------------------------------------------------------------------
// Handle mouse button presses
//--------------------------------------------------------------------------------------
void CALLBACK OnMouse( bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown,
                       bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta,
                       int xPos, int yPos, void* pUserContext )
{

}


//--------------------------------------------------------------------------------------
// Call if device was removed.  Return true to find a new device, false to quit
//--------------------------------------------------------------------------------------
bool CALLBACK OnDeviceRemoved( void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Initialize everything and go into a render loop
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device (either D3D9 or D3D10) 
    // that is available on the system depending on which D3D callbacks are set below

    // Set general DXUT callbacks
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackMouse( OnMouse );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackDeviceRemoved( OnDeviceRemoved );

    // Set the D3D10 DXUT callbacks. Remove these sets if the app doesn't need to support D3D10
    DXUTSetCallbackD3D10DeviceAcceptable( IsD3D10DeviceAcceptable );
    DXUTSetCallbackD3D10DeviceCreated( OnD3D10CreateDevice );
    DXUTSetCallbackD3D10SwapChainResized( OnD3D10ResizedSwapChain );
    DXUTSetCallbackD3D10FrameRender( OnD3D10FrameRender );
    DXUTSetCallbackD3D10SwapChainReleasing( OnD3D10ReleasingSwapChain );
    DXUTSetCallbackD3D10DeviceDestroyed( OnD3D10DestroyDevice );

    // Initialization
	AllocConsole();
	SetConsoleTitle(L"SuperConsole");
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN | FOREGROUND_INTENSITY); // Mega-cool green text ;)
	_cprintf("Starting application...GO!\n");
	_cprintf("Maximum simultaneous render targets supported: %i\n", D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT);

	instance = DeferredApp::instance();

    DXUTInit( true, false, NULL ); // Parse the command line, show NO msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"DeferredRendererrrrr" );
	DXUTSetIsInGammaCorrectMode(false); // Tell DX that I am not doing gamma correction :)
    DXUTCreateDevice( true, 800, 600 );
    DXUTMainLoop(); // Enter into the DXUT render loop

    // Perform any application-level cleanup here
	FreeConsole();

	// Free up the singleton
	delete instance;
	instance = NULL;

    return DXUTGetExitCode();
}


