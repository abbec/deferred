#ifndef _DEFERRED_APP
#define _DEFERRED_APP
#include "../Scene/Scene.h"

class DeferredApp
{
public:
	static DeferredApp *instance();

	~DeferredApp();

	HRESULT initScene(ID3D10Device* device);
	bool initBuffers(ID3D10Device* device, const DXGI_SURFACE_DESC*);
	void render(ID3D10Device* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext);

	inline Scene *getScene() { return &_scene; }

	void update(double fTime, float fElapsedTime, void* pUserContext);
	void render_to_quad();

	LRESULT handle_messages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	static DeferredApp *inst;
	DeferredApp();

	ID3D10Device *_device;
	double _time;
	double _elapsed_time;
	void *_user_context;

	ID3D10Effect *_effect;
	ID3D10EffectTechnique *_geometry_stage, *_render_to_quad;
	ID3D10InputLayout *_layout, *_quad_layout;
	ID3D10EffectVariable *_g_buffer_SR;

	ID3D10Buffer *_quad_VB;

	Scene _scene;

	// G-buffer
	ID3D10RenderTargetView *_g_buffer_views[2];
	ID3D10ShaderResourceView *_g_buffer_SRV[2];
	ID3D10Texture2D *_g_normals, *_g_depth;


	void geometry_stage();
};
#endif //_DEFERRED_APP