#include "DXUT.h"
#include "Object.h"
#include <fstream>
#include <conio.h>
#include <cassert>
#include <string>

using namespace Deferred;

const D3D10_INPUT_ELEMENT_DESC Object::LAYOUT[] =
	{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D10_INPUT_PER_VERTEX_DATA, 0 },
} ;

const UINT Object::NUM_LAYOUT_ELMS = sizeof(LAYOUT) / sizeof(LAYOUT[0]);

Object::Object() :
_mesh(NULL)
{
	D3DXMatrixIdentity(&_transform);
}

Object::~Object()
{
	// Clean up D3D resources
	SAFE_RELEASE(_mesh);

	std::vector<Material *>::iterator it = _materials.begin();

	while (it != _materials.end())
	{
		delete *it;
		it++;
	}

	SAFE_DELETE_ARRAY(_attrib_table);
}

bool Object::read_from_obj(ID3D10Device *device, std::string filename)
{
	WCHAR str_command[256] = {0};
	WCHAR material_file[256] = {0};
    std::wifstream infile(filename);

	DWORD current_subset = 0;

	// Temp arrays for storing pos, tex coords and normals
	std::vector<D3DXVECTOR3> positions;
	std::vector<D3DXVECTOR2> tex_coords;
	std::vector<D3DXVECTOR3> normals;

	assert(device != NULL && "Do not send in null device!");

	_device = device;

	if (!infile)
	{
		_cprintf("The file %s could not be found!", filename.c_str());
		return false;
	}

	_cprintf("Reading file: %s ...\n", filename.c_str());

	while(infile)
	{
		infile >> str_command;

		if(wcscmp( str_command, L"#" ) == 0) // Ignore comments
		{

		}
		else if (wcscmp(str_command, L"v") == 0) // Vertex position
		{
			float x, y, z;
            infile >> x >> y >> z;
			positions.push_back(D3DXVECTOR3(x, y, z));
		}
		else if (wcscmp(str_command, L"vn") == 0) // Vertex normal
		{
			float x, y, z;
			infile >> x >> y >> z;
			normals.push_back(D3DXVECTOR3(x, y, z));
		}
		else if (wcscmp(str_command, L"vt") == 0) // Texture coords
		{
			float u, v;
			infile >> u >> v;
			tex_coords.push_back(D3DXVECTOR2(u,v));
		}
		else if (wcscmp(str_command, L"f") == 0) // face
		{
			UINT pos_index, tex_index, normal_index;
			Vertex vertex;
			DWORD ind[3];

			for (int i = 0; i < 3; i++)
			{
				ZeroMemory(&vertex, sizeof(Vertex));

				// Position index
				infile >> pos_index;
				vertex.position = positions.at(pos_index-1); // 1-based arrays

				if(infile.peek() == '/')
                {
                    infile.ignore();

                    if(infile.peek() != '/')
                    {
                        // Optional texture coordinate
                        infile >> tex_index;
                        vertex.texcoord = tex_coords.at(tex_index - 1);
                    }

                    if(infile.peek() == '/')
                    {
                        infile.ignore();

                        // Optional vertex normal
                        infile >> normal_index;
                        vertex.normal = normals.at(normal_index -1);
                    }
                }

				// Check if the vertex is already in the vertex list
				std::map<UINT, VertexEntry>::iterator it = _unique_verts.find(pos_index);
				VertexEntry ve;
				ve.vertex = vertex;
				DWORD index = 0;
				if (it == _unique_verts.end())
				{
					_vertex_list.push_back(vertex);
					ve.index = _vertex_list.size()-1;
					_unique_verts.insert(std::make_pair<UINT, VertexEntry>(pos_index, ve));
					index = ve.index;
				}
				else
					index = (*it).second.index;

				assert(index <_vertex_list.size() && "index larger than size of vertex list.");

				ind[i] = index; // OBJ is in clockwise order???
			}

			// Reverse vertex order (see above comment)
			_index_list.push_back(ind[2]);
			_index_list.push_back(ind[1]);
			_index_list.push_back(ind[0]);

			// Add a subset
			_attributes.push_back(current_subset);
		}
		else if (wcscmp(str_command, L"mtllib") == 0)
		{
			infile >> material_file;
		}
		else if (wcscmp(str_command, L"usemtl") == 0)
		{
			WCHAR material_name[200] = {0};
			infile >> material_name;

			// Check if the material exists
			std::map<std::wstring, UINT>::iterator it;

			it = _material_ids.find(std::wstring(material_name));

			if (it != _material_ids.end()) // The material exists
			{
				current_subset = it->second;
			}
			else // It is a new material
			{
				current_subset = _materials.size();
				_material_ids.insert(std::make_pair<std::wstring, UINT>(std::wstring(material_name), current_subset));
				_materials.push_back(new Material());
			}
		}
		else
		{
			 //_cwprintf(L"Unimplemented command: %s \n", str_command);
		}

		infile.ignore(1000, '\n');
	}

	infile.close();

	_cwprintf(L"Done! Reading material file %s \n", material_file);

	read_materials(std::wstring(material_file));
	return set_up_mesh();
}

void Object::render()
{
	_mesh->DrawSubset(0);
}

void Object::render_subset(UINT subset_id)
{
	_mesh->DrawSubset(subset_id);
}

bool Object::set_up_mesh()
{
	HRESULT hr = D3DX10CreateMesh(_device, LAYOUT, NUM_LAYOUT_ELMS,
		LAYOUT[0].SemanticName, _vertex_list.size(),
		_index_list.size() / 3, D3DX10_MESH_32_BIT, &_mesh);

	if (FAILED(hr))
		return false;

	//int num_verts = _vertex_list.size();
	int num_indices = _index_list.size();

	// Set up vertex list
	hr = _mesh->SetVertexData(0, (void*) (_vertex_list.data()));
	_vertex_list.clear();

	if (FAILED(hr))
		return false;

	// Set up index list
	hr = _mesh->SetIndexData((void*) (_index_list.data()), num_indices);
	_index_list.clear();

	if (FAILED(hr))
		return false;

	// Set the attribute data
	_mesh->SetAttributeData( (UINT*)_attributes.data() );
	_attributes.clear();

	_mesh->GenerateAdjacencyAndPointReps( 1e-6f );
	_mesh->Optimize( D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_VERTEXCACHE, NULL, NULL );

	// Set attributes
	_mesh->GetAttributeTable( NULL, &_num_attrib_table_entries );
    _attrib_table = new D3DX10_ATTRIBUTE_RANGE[_num_attrib_table_entries];
    _mesh->GetAttributeTable(_attrib_table, &_num_attrib_table_entries );

	// Commit the mesh to device
	hr = _mesh->CommitToDevice();

	if (FAILED(hr))
		return false;

	_cprintf("Done! Number of vertices: %i, faces: %i\n", _mesh->GetVertexCount(), _mesh->GetFaceCount());

	return true;
}

bool Object::read_materials(std::wstring filename)
{
	std::wifstream infile((L"Media\\" + filename).c_str());

	if (!infile)
	{
		_cwprintf(L"ERROR: Could not locate mtl file: %s \n", filename.c_str());
		infile.close();
		return false;
	}

	WCHAR command[256] = {0};
	Material *active_material = NULL;

	while (infile)
	{
		if(wcscmp(command, L"newmtl") == 0)
        {
			WCHAR mtl_name[256] = {0};
			infile >> mtl_name;
			std::map<std::wstring, UINT>::iterator it = _material_ids.find(std::wstring(mtl_name));

			if (it == _material_ids.end())
				continue;
			
			active_material = _materials.at(it->second);
		}

		if(wcscmp(command, L"#" ) == 0)
        { /* Comment */ }
		else if(wcscmp(command, L"Ka") == 0)
        {
            // Ambient color
            float r, g, b;
            infile >> r >> g >> b;
            active_material->set_ambient_color(D3DXVECTOR3( r, g, b ));
        }
        else if(wcscmp(command, L"Kd") == 0)
        {
            // Diffuse color
            float r, g, b;
            infile >> r >> g >> b;
			active_material->set_diffuse_color(D3DXVECTOR3( r, g, b ));
        }
        else if(wcscmp(command, L"Ks") == 0)
        {
            // Specular color
            float r, g, b;
            infile >> r >> g >> b;
			active_material->set_specular_color(D3DXVECTOR3( r, g, b ));
        }
        else if(wcscmp(command, L"d" ) == 0 ||
                wcscmp(command, L"Tr" ) == 0)
        {
            // Alpha
			float a;
			infile >> a;
            active_material->set_alpha(a);
        }
        else if(wcscmp(command, L"Ns" ) == 0)
        {
            // Shininess
            float spec_power;
            infile >> spec_power;
			active_material->set_specular_power(spec_power);
        }
        else if(wcscmp(command, L"illum" ) == 0)
        {
            // Specular on/off
            int illumination;
            infile >> illumination;
			active_material->set_specular( illumination == 2 );
        }
        else if(wcscmp(command, L"map_Kd" ) == 0)
        {
            // Texture
			WCHAR texture_file[256] = {0};
            infile >> texture_file;

			active_material->create_texture(_device, std::wstring(texture_file));
        }

        else
        {
            // Unimplemented or unrecognized command
        }

		infile.ignore(1000, '\n');
	}

	infile.close();

	return true;
}