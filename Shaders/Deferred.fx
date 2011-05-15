//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
matrix World;
matrix View;
matrix WorldViewInverse;
matrix Projection;

float SpecularIntensity;

Texture2D AlbedoTexture;

//--------------------------------------------------------------------------------------
Texture2D Normals;
Texture2D Depth;

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Normal : NORMAL;
	float2 TexCoord : TEXCOORD;
};

struct VS_INPUT
{	
	float3 Pos : POSITION;
	float3 Normal : NORMAL;
	float2 TexCoord : TEXCOORD;
};

struct PS_OUTPUT
{
	float4 normal : SV_TARGET0;
	float4 depth : SV_TARGET1;
};

struct VS_SCREENOUTPUT
{
    float4 Position   : SV_POSITION; // vertex position  
};

// Texture sampler
SamplerState samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT GBufferVS( VS_INPUT input )
{
    VS_OUTPUT output = (VS_OUTPUT)0;

	// Transform into world coordinates
	output.Pos = float4(input.Pos, 1.0);
    output.Pos = mul(output.Pos , World );
	output.Pos = mul(output.Pos, View);
	output.Pos = mul(output.Pos, Projection);
	// View space normal
	output.Normal = float4(input.Normal, 0.0);
	output.Normal = normalize(mul(output.Normal, WorldViewInverse));
    output.TexCoord = input.TexCoord;

    return output;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
PS_OUTPUT GBufferPS( VS_OUTPUT input ) : SV_Target
{
	PS_OUTPUT output;

	// Render to g_buffer
	output.normal = normalize(input.Normal);
	float4 pos = input.Pos;
	output.depth = pos.z/pos.w;

	return output;
}


//--------------------------------------------------------------------------------------
// Render to quad
//--------------------------------------------------------------------------------------
VS_SCREENOUTPUT ScreenVS(float4 pos : POSITION)
{
	VS_SCREENOUTPUT Output;
    Output.Position = pos;

    return Output;
}


float4 ScreenPS(VS_SCREENOUTPUT Input) : SV_Target
{
	float4 pos = Input.Position;
	//return float4(pos.x, pos.y, 0.0, 0.0);
	return Normals.Load(float3(pos.xy, 0));
}

//--------------------------------------------------------------------------------------
technique10 GeometryStage
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, GBufferVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, GBufferPS() ) );
    }
}

technique10 RenderToQuad
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, ScreenVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, ScreenPS() ) );
    }
}