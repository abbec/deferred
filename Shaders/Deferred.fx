//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
matrix World;
matrix View;
matrix WorldViewInverse;
matrix Projection;


//--------------------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Normal : NORMAL0;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VS( float4 Pos : POSITION, float3 Normal : NORMAL )
{
    VS_OUTPUT output = (VS_OUTPUT)0;

	matrix obj2world = mul(World, View);

	obj2world = mul(obj2world, Projection);
	// Transform into world coordinates
    output.Pos = mul( Pos, obj2world );

	// View space normal
	output.Normal = float4(Normal, 0.0);
	output.Normal = normalize(mul(output.Normal, WorldViewInverse));
    
    return output;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( VS_OUTPUT input ) : SV_Target
{
	float4 normal = normalize(input.Normal);

    return float4((normal * 0.5f + 0.5));
}


//--------------------------------------------------------------------------------------
technique10 Render
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
    }
}