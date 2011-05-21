//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------

cbuffer NeverChange
{
	float FarClipDistance;
	float4 Ambient;
}

cbuffer everyFrame
{
	matrix World;
	matrix View;
	matrix WorldViewInverse;
	matrix Projection;
	float SpecularIntensity;
	float SpecularRoughness;
}

//--------------------------------------------------------------------------------------

Texture2D AlbedoTexture;

// Far clipping plane corners.
// Needed for view space position
// reconstruction.
float3 FarPlaneCorners[4];

// -----GBuffer--- //
Texture2D Normals;
Texture2D Depth;
Texture2D Albedo;
Texture2D SpecularInfo;
// -------------- //

Texture2D FinalImage;

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
	float4 VS_Pos : POSITION0;
    float4 Normal : NORMAL;
	float2 TexCoord : TEXCOORD;
};

struct VS_INPUT
{	
	float3 Pos : POSITION;
	float3 Normal : NORMAL;
	float2 TexCoord : TEXCOORD;
};

struct PS_MRT_OUTPUT
{
	float4 normal : SV_TARGET0;
	float4 depth : SV_TARGET1;
	float4 albedo : SV_TARGET2;
	float4 specularInfo : SV_TARGET3;
};

struct VS_SCREENOUTPUT
{
    float4 Position   : SV_POSITION; // vertex position
	float3 FrustumCorner : TEXCOORD0;
	float2 TexCoords : TEXCOORD1;
};

// Texture sampler
SamplerState samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

//--------------------------------------------------------------------------------------
// G Buffer Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT GBufferVS( VS_INPUT input )
{
    VS_OUTPUT output = (VS_OUTPUT)0;

	// Transform position to view space
	output.Pos = float4(input.Pos, 1.0);
    output.Pos = mul(output.Pos , World );
	output.Pos = mul(output.Pos, View);
	output.VS_Pos = output.Pos; 
	output.Pos = mul(output.Pos, Projection);

	// View space normal
	output.Normal = float4(input.Normal, 1.0);
	output.Normal = normalize(mul(output.Normal, WorldViewInverse));
    
	// Pass texture coordinates on
	output.TexCoord = input.TexCoord;

    return output;
}


//--------------------------------------------------------------------------------------
// G Buffer Pixel Shader
//--------------------------------------------------------------------------------------
PS_MRT_OUTPUT GBufferPS( VS_OUTPUT input ) //: SV_Target
{
	PS_MRT_OUTPUT output;

	// Render to g_buffer
	output.normal = float4(normalize(input.Normal).xyz, 1.0);
	float depth = -input.VS_Pos.z/FarClipDistance;
	output.depth = float4(depth, depth, depth, 1.0);
	output.albedo = float4(AlbedoTexture.Sample( samLinear, input.TexCoord ).rgb, 1.0);
	output.specularInfo = float4(SpecularIntensity, SpecularRoughness, 1.0, 1.0);

	return output;
}

VS_SCREENOUTPUT AmbientLightVS(float4 pos : POSITION, float3 texCoords : TEXCOORD0)
{
	VS_SCREENOUTPUT Output;
    Output.Position = pos;

	Output.FrustumCorner = FarPlaneCorners[texCoords.z];
	Output.TexCoords = texCoords.xy;

    return Output;
}

float4 AmbientLightPS(VS_SCREENOUTPUT Input) : SV_TARGET0
{
	float3 color = Albedo.Sample(samLinear, Input.TexCoords.xy);
	return Ambient * float4(color, 0.0);
	//return float4(1.0, 0.0, 0.0, 1.0);
}

//--------------------------------------------------------------------------------------
// Render to quad
//--------------------------------------------------------------------------------------
VS_SCREENOUTPUT ScreenVS(float4 pos : POSITION, float3 texCoords : TEXCOORD)
{
	VS_SCREENOUTPUT Output;
    Output.Position = pos;

	Output.FrustumCorner = FarPlaneCorners[texCoords.z];
	Output.TexCoords = texCoords.xy;

    return Output;
}


float4 ScreenPS(VS_SCREENOUTPUT Input) : SV_Target
{
	float4 pos = Input.Position;
	float depth = Depth.Load(float3(pos.xy, 0)).r;

	// Reconstruct view space position
	float4 position = float4(depth * Input.FrustumCorner, 0.0);

	return FinalImage.Load(float3(pos.xy, 0));
	//return float4(0.1, 0.1, 0.1, 1.0);
	//return AlbedoTexture.Sample(samLinear, Input.TexCoords);
}

float4 ScreenNormalsPS(VS_SCREENOUTPUT Input) : SV_Target
{
	float4 pos = Input.Position;

	return Normals.Load(float3(pos.xy, 0));
}

float4 ScreenDepthPS(VS_SCREENOUTPUT Input) : SV_Target
{
	float4 pos = Input.Position;

	return Depth.Load(float3(pos.xy, 0));
}

float4 ScreenAlbedoPS(VS_SCREENOUTPUT Input) : SV_Target
{
	float4 pos = Input.Position;

	return Albedo.Load(float3(pos.xy, 0));
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


technique10 AmbientLight
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, AmbientLightVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, AmbientLightPS() ) );
    }
}

// For rendering different states

technique10 RenderToQuad // Final compositing
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, ScreenVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, ScreenPS() ) );
    }
}

technique10 RenderNormalsToQuad
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, ScreenVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, ScreenNormalsPS() ) );
    }
}

technique10 RenderDepthToQuad
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, ScreenVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, ScreenDepthPS() ) );
    }
}

technique10 RenderAlbedoToQuad
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, ScreenVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, ScreenAlbedoPS() ) );
    }
}