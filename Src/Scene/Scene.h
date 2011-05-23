#ifndef _SCENE_H
#define _SCENE_H
#include "DXUTcamera.h"
#include "Object.h"
#include "Light.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include <vector>

struct DXUTVertex
{
    D3DXVECTOR3 pos;
    D3DXVECTOR3 norm;
};


class Scene
{
public:
	Scene();
	~Scene();

	HRESULT init(ID3D10Device *device, ID3D10Effect *effect);
	HRESULT set_view(const DXGI_SURFACE_DESC *back_buffer_desc);
	void render(ID3D10Device *device, ID3D10EffectPass *pass);
	void render_skybox(ID3D10Device *device);
	void draw_lights(ID3D10Device *device);
	void rotate(D3DXVECTOR3 &at);

	void update(double fTime, float fElapsedTime, void* pUserContext);

	LRESULT handle_messages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:

// Objects
	std::vector<Deferred::Object *> _objects;
	std::vector<Deferred::Light *> _lights;

	ID3D10Effect *_effect;
	
	ID3D10EffectShaderResourceVariable *_texture_SR;
	ID3D10EffectMatrixVariable *_worldVariable;
	ID3D10EffectVectorVariable *_far_plane_corners_variable;
	ID3D10EffectMatrixVariable *_viewVariable;
	ID3D10EffectMatrixVariable *_projectionVariable;
	ID3D10EffectMatrixVariable *_wv_inverse;
	ID3D10EffectScalarVariable *_spec_intensity_var;

	D3DXMATRIX _world;
	D3DXMATRIX _view;
	D3DXMATRIX _projection;
	D3DXMATRIX _world_view_inv;

	CModelViewerCamera _camera;

	ID3DX10Mesh *_skybox;
	ID3D10ShaderResourceView *_skybox_texture_RV;

	D3DXVECTOR3 _ambient_color;
	
	void bump_shader_variables(const D3DXMATRIX *translation);
	void bump_light_variables(Deferred::Light *l);
	
};
#endif // _SCENE_H