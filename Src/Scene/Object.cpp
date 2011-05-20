#include "DXUT.h"
#include "Object.h"
#include <fstream>
#include <conio.h>
#include <cassert>

using namespace Deferred;

const D3D10_INPUT_ELEMENT_DESC Object::LAYOUT[] =
	{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D10_INPUT_PER_VERTEX_DATA, 0 },
} ;

const UINT Object::NUM_LAYOUT_ELMS = sizeof(LAYOUT) / sizeof(LAYOUT[0]);

Object::Object() :
_mesh(NULL), _texture_RV(NULL)
{
	D3DXMatrixIdentity(&_transform);
}

Object::~Object()
{
	// Clean up D3D resources
	if (_mesh)
		_mesh->Release();

	if (_texture_RV)
		_texture_RV->Release();
}

bool Object::read_from_obj(ID3D10Device *device, std::string filename)
{
	WCHAR str_command[256] = {0};
    std::wifstream infile(filename);

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

			// Reverse vertex order
			_index_list.push_back(ind[2]);
			_index_list.push_back(ind[1]);
			_index_list.push_back(ind[0]);
		}
		else if (wcscmp(str_command, L"mtllib") == 0)
		{
			//_cprintf("Materials not implemented yet...\n");
		}
		else if (wcscmp(str_command, L"usemtl") == 0)
		{

		}
		else
		{
			 //_cwprintf(L"Unimplemented command: %s \n", str_command);
		}

		infile.ignore(1000, '\n');
	}

	_cprintf("Done! Setting up mesh...\n");

	return set_up_mesh();
}

void Object::render()
{
	_mesh->DrawSubset(0);
}

bool Object::set_up_mesh()
{
	HRESULT hr = D3DX10CreateMesh(_device, LAYOUT, NUM_LAYOUT_ELMS,
		LAYOUT[0].SemanticName, _vertex_list.size(),
		_index_list.size() / 3, D3DX10_MESH_32_BIT, &_mesh);

	if (FAILED(hr))
		return false;

	int num_verts = _vertex_list.size();
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

	_mesh->GenerateAdjacencyAndPointReps( 1e-6f );

	// Set attributes
    DWORD dwNumAttr = 1;
    D3DX10_ATTRIBUTE_RANGE* pAttr = new D3DX10_ATTRIBUTE_RANGE[dwNumAttr];
    if( !pAttr )
        return false;

    pAttr[0].AttribId = 0;
    pAttr[0].FaceStart = 0;
    pAttr[0].FaceCount = num_indices / 3;
    pAttr[0].VertexStart = 0;
    pAttr[0].VertexCount = num_verts;
    _mesh->SetAttributeTable( pAttr, dwNumAttr );
    SAFE_DELETE_ARRAY( pAttr );

	// Commit the mesh to device
	hr = _mesh->CommitToDevice();

	if (FAILED(hr))
		return false;

	_cprintf("Done! Number of vertices: %i, faces: %i\n", _mesh->GetVertexCount(), _mesh->GetFaceCount());

	// Load texture
	if ( FAILED( D3DX10CreateShaderResourceViewFromFile(_device, L"Media\\Textures\\fur2.jpg", NULL, NULL, &_texture_RV, NULL )))
		  _cprintf("Could not load texture!\n");

	return true;
}