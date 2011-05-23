#ifndef _DEFERRED_APP
#define _DEFERRED_APP
#include "../Scene/Scene.h"
#include "../Util/Hud.h"


struct QuadVertex
{
	D3DXVECTOR3 position;
	D3DXVECTOR3 tex_plus_frustum;
};

class DeferredApp
{
public:
	static DeferredApp *instance();

	// Rendering states
	const enum states {NORMALS = 1, DEPTH = 2, ALBEDO = 3, FINAL = 5};
	const enum {GBUFFER_SIZE = 3};
	const enum {PBUFFER_SIZE = 2};

	~DeferredApp();

	HRESULT initScene(ID3D10Device* device);
	HRESULT initBuffers(ID3D10Device* device, const DXGI_SURFACE_DESC*);
	void clean_buffers();
	void render(ID3D10Device* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext);

	inline Scene *getScene() { return &_scene; }

	void update(double fTime, float fElapsedTime, void* pUserContext);
	void render_to_quad();

	void set_render_state(UINT state) { _render_state = state; }
	void forward_rendering() { _deferred = false; }
	void deferred_rendering() { _deferred = true; }

	LRESULT handle_messages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	static DeferredApp *inst;
	DeferredApp();

	Deferred::Hud *_hud;

	ID3D10Device *_device;
	double _time;
	double _elapsed_time;
	void *_user_context;

	ID3D10Effect *_effect;
	ID3D10EffectTechnique *_geometry_stage, *_render_to_quad, *_render_normals_to_quad;
	ID3D10EffectTechnique *_render_depth_to_quad, *_render_albedo_to_quad;
	ID3D10InputLayout *_layout, *_quad_layout;

	ID3D10Buffer *_quad_VB;

	Scene _scene;
	UINT _render_state;
	bool _deferred;
	ID3D10RenderTargetView *_backbuffer;
	ID3D10DepthStencilView *_depth_stencil;

	ID3D10RasterizerState *_rs_state, *_rs_default_state;


	// G-buffer
	ID3D10RenderTargetView *_g_buffer_views[GBUFFER_SIZE];
	ID3D10ShaderResourceView *_g_buffer_SRV[GBUFFER_SIZE];
	ID3D10Texture2D *_g_textures[GBUFFER_SIZE];

	// P-buffer
	ID3D10RenderTargetView *_p_buffer_views[PBUFFER_SIZE];
	ID3D10ShaderResourceView *_p_buffer_SRV[PBUFFER_SIZE];
	ID3D10Texture2D *_p_textures[PBUFFER_SIZE];

	void geometry_stage();
	void lighting_stage();
};
#endif //_DEFERRED_APP