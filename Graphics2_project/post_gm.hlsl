
struct OutputVertex
{
    float4 position : SV_Position;
    float3 campos : CAMPOS;
    float2 tex : TEX;
    int mode : MODE;
};

[maxvertexcount(3)]
void main(
	triangle OutputVertex input[3],
	inout TriangleStream<OutputVertex> output
)
{
	for (uint i = 0; i < 3; i++)
	{
        OutputVertex element;
		element.position = input[i].position;
        element.tex = input[i].tex;
        element.campos = input[i].campos;
        element.mode = input[i].mode;
		output.Append(element);
	}
}