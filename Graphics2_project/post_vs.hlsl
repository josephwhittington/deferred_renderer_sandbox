
struct InputVertex
{
    float3 position : POSITION;
    float2 tex : TEX;
};

struct OutputVertex
{
    float4 position : SV_Position;
    float3 campos : CAMPOS;
    float2 tex : TEX;
    int mode : MODE;
};

cbuffer CAMERAPOS
{
    float4 camera_position;
    int mode;
};

OutputVertex main(InputVertex input)
{
    OutputVertex output;
    
    output.position = float4(input.position, 1);
    output.campos = camera_position;
    output.tex = input.tex;
    output.mode = mode;
    
    return output;
}