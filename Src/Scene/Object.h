#ifndef _OBJECT_H
#define _OBJECT_H

#include <iostream>
#include <vector>
#include <map>

#include "Material.h"

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
		void render_subset(UINT subset_id);

		const Material *get_subset_material(UINT subset_id) const { return _materials.at( _attrib_table[subset_id].AttribId ); }

		UINT get_num_subsets() { return _num_attrib_table_entries; }

		void set_transform(D3DXMATRIX *transform) { _transform = *transform; }
		const D3DXMATRIX *get_transform() const { return &_transform; }

	private:

		bool set_up_mesh();
		bool read_materials(std::wstring filename);

		ID3DX10Mesh *_mesh;
		ID3D10Device *_device;

		std::vector<Vertex> _vertex_list;
		std::vector<DWORD> _index_list;
		std::vector<DWORD> _attributes;
		std::map<UINT, VertexEntry> _unique_verts;

		std::vector<Material*> _materials;
		std::map<std::wstring, UINT> _material_ids;

		UINT _num_attrib_table_entries;
		D3DX10_ATTRIBUTE_RANGE *_attrib_table;

		D3DXMATRIX _transform;

	};

}
#endif