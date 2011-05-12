#ifndef _SCENE_H
#define _SCENE_H

class Scene
{
public:
	Scene();
	~Scene();

	HRESULT init(ID3D10Device *device);
	void render(ID3D10Device *device);
	void rotate(D3DXVECTOR3 &at);

private:

	// Meshes
	ID3DX10Mesh *_sphere;
	ID3DX10Mesh *_teapot;


	ID3D10InputLayout *_layout;
	ID3D10Effect *_effect;
	ID3D10EffectMatrixVariable *_worldVariable;
	ID3D10EffectMatrixVariable *_viewVariable;
	ID3D10EffectMatrixVariable *_projectionVariable;
	ID3D10EffectMatrixVariable *_wv_inverse;
	ID3D10EffectScalarVariable *_spec_intensity_var;

	ID3D10EffectTechnique *_technique;

	D3DXMATRIX _world;
	D3DXMATRIX _view;
	D3DXMATRIX _projection;
	D3DXMATRIX _world_view_inv;
	
};
#endif // _SCENE_H