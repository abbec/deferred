#ifndef _SCENE_H
#define _SCENE_H
#include "DXUTcamera.h"
#include "Object.h"
#include <vector>

class Scene
{
public:
	Scene();
	~Scene();

	HRESULT init(ID3D10Device *device, ID3D10Effect *effect);
	void render(ID3D10Device *device);
	void rotate(D3DXVECTOR3 &at);

	void update(double fTime, float fElapsedTime, void* pUserContext);

	LRESULT handle_messages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:

// Objects
	std::vector<Deferred::Object *> _objects;

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
void bump_shader_variables();
	
};
#endif // _SCENE_H