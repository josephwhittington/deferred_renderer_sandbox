

struct OutputVertex
{
    float4 position : SV_Position;
    float3 worldpos : WORLDPOS;
    float3 normal : NORMAL;
    float3 vsnormal : VIEWSPACENORMAL;
    float3 campos : CAMPOS;
    float2 tex : TEXCOORD;
};

struct ShaderOut
{
    float4 lightAccumulation : SV_Target0;
    float4 diffuse : SV_Target1;
    float4 specular : SV_Target2;
    float4 normal : SV_Target3;
    float4 position : SV_Target4;
};

ShaderOut main(OutputVertex input)
{   
    ShaderOut output;
    // Compute lighting values

    output.diffuse = float4(1, 1, 1, 1);
    output.specular = output.diffuse * .3;
    output.normal = float4(input.vsnormal, 1);
    output.lightAccumulation = float4(0, 0, 0, 1);
    output.position = float4(input.worldpos, 1);
    
    return output;
}