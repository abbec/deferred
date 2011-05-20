#ifndef _OBJECT_H
#define _OBJECT_H

#include <iostream>
#include <vector>
#include <map>

namespace Deferred
{

	struct Vertex
	{
		D3DXVECTOR3 position;
		D3DXVECTOR3 normal;
		D3DXVECTOR2 texcoord;
	};

	struct VertexEntry
	{
		DWORD index;
		Vertex vertex;
	};

	class Object
	{
	public:
		// Define the input layout
		static const D3D10_INPUT_ELEMENT_DESC LAYOUT[];
		static const UINT NUM_LAYOUT_ELMS;

		Object();
		~Object();

		bool read_from_obj(ID3D10Device *device, std::string filename);
		void render();
		ID3D10ShaderResourceView *get_texture() { return _texture_RV; }

		void set_transform(D3DXMATRIX transform) { _transform = transform; }
		const D3DXMATRIX *get_transform() { return &_transform; }

	private:

		bool set_up_mesh();

		ID3DX10Mesh *_mesh;
		ID3D10Device *_device;

		std::vector<Vertex> _vertex_list;
		std::vector<DWORD> _index_list;
		std::map<UINT, VertexEntry> _unique_verts;

		ID3D10ShaderResourceView *_texture_RV;

		D3DXMATRIX _transform;

	};

}
#endif