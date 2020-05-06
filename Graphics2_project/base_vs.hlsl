
// Use row major matrices
#pragma pack_matrix(row_major)

struct InputVertex
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
};

cbuffer SHADER_VARIABLES : register(b0)
{
    float4x4 worldMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4 camera_position;
};

struct OutputVertex
{
    float4 position : SV_Position;
    float3 worldpos : WORLDPOS;
    float3 normal : NORMAL;
    float3 vsnormal : VIEWSPACENORMAL;
    float3 campos : CAMPOS;
    float2 tex : TEXCOORD;
};

OutputVertex main(InputVertex input)
{
    OutputVertex output;
    
    output.position = mul(float4(input.position, 1), worldMatrix);
    output.worldpos = output.position.xyz;
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
    output.normal = normalize(mul(float4(input.normal, 0), worldMatrix).xyz);
    output.vsnormal = normalize(mul(float4(output.normal, 0), viewMatrix).xyz);
    output.campos = camera_position.xyz;
    output.tex = input.tex;
    
	return output;
}