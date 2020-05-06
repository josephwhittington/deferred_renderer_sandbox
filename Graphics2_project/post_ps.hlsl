Texture2D t_depth : register(t0);
Texture2D t_diffuse : register(t1);
Texture2D t_normal : register(t2);
Texture2D t_specular : register(t3);
Texture2D t_position : register(t4);
SamplerState diffuseSampler : register(s0);

struct OutputVertex
{
    float4 position : SV_Position;
    float3 campos : CAMPOS;
    float2 tex : TEX;
    int mode : MODE;
};

struct PixelData
{
    float depth;
    float3 diffuse;
    float3 normal;
    float3 specular;
    float3 position;
};

#define LIGHTTYPE_DIRECTIONAL 0
#define LIGHTTYPE_POINT 1
#define	LIGHTTYPE_SPOT  2
#define LIGHT_COUNT 250

#define SHININESS 1000

struct Light
{
    float4 position, lightDirection;
    float4 ambient, diffuse, specular;
    unsigned int lightType;
    float ambientIntensity, diffuseIntensity, specularIntensity;
    float cosineInnerCone, cosineOuterCone;
    float lightRadius;
    int lightOn;
};

cbuffer LIGHTS : register(b0)
{
    Light lights[LIGHT_COUNT];
};

float4 computeDirectionalLight(PixelData data, Light dlight, OutputVertex input)
{
    // Hard coded values
    float specularShininess = 100;
    // Texture color
    float4 color = t_diffuse.Sample(diffuseSampler, input.tex);
    float3 world_pos = t_position.Sample(diffuseSampler, input.tex).xyz;
    
    // Ambient color
    float3 ambientColor = saturate(color.rgb * dlight.ambientIntensity);
    
    // Diffuse color
    float3 normal = normalize(t_normal.Sample(diffuseSampler, input.tex).xyz);
    float3 lightDirection = normalize(dlight.lightDirection.xyz);
    float diffuseFactor = saturate(dot(normal, -lightDirection));
    float3 diffuseColor = saturate(diffuseFactor * dlight.diffuseIntensity * dlight.diffuse.rgb);
    
    // Specular color
    float3 viewDirection = normalize(input.campos - world_pos);
    float3 halfVector = normalize(-lightDirection + viewDirection);
    float specularFactor = pow(max(dot(normal, halfVector), 0), specularShininess);
    float3 specularColor = saturate(specularFactor * dlight.specularIntensity * dlight.specular.rgb);
    
    // Combine phong components
    color = saturate(float4((ambientColor + diffuseColor + specularColor) * color.rgb, color.a));
    
    return color;
}

float4 computePointLight(PixelData data, Light plight, OutputVertex input)
{
    // Hard coded values
    float specularShininess = 100;
    // Texture color
    float4 color = float4(data.diffuse, 1);
    float3 world_pos = data.position;
    
    // Ambient color
    float distance = length(plight.position.xyz - world_pos);
    if (plight.lightRadius * 2.1 < distance)
        return float4(0, 0, 0, 1);
    
    float3 ambientColor = saturate(color.rgb * plight.ambientIntensity);
    
    // Diffuse color
    float3 normal = normalize(data.normal);
    float3 lightDirection = (plight.position.xyz - world_pos);
    float diffuseFactor = saturate(dot(normal, lightDirection));
    float3 diffuseColor = saturate(diffuseFactor * plight.diffuseIntensity * plight.diffuse);
    
    // Specular color
    // The view direction needs to be added to the positive light direction here
    float3 viewDirection = normalize(input.campos - world_pos);
    float3 halfVector = normalize(lightDirection + viewDirection);
    float specularFactor = pow(max(dot(normal, halfVector), 0), specularShininess);
    float3 specularColor = saturate(specularFactor * plight.specularIntensity * plight.specular);
    
    // Attenuation
    float attenuation = 1.0 - saturate(distance / plight.lightRadius);
    attenuation *= attenuation;
    
    // Apply attenuation
    diffuseColor = saturate(diffuseColor * attenuation);
    specularColor = saturate(specularColor * attenuation);
    
    // Combine phong components
    color = saturate(float4((ambientColor + diffuseColor + specularColor) * color.rgb, color.a));
    return color;
}


float4 ComputeLight(PixelData data, OutputVertex input)
{
    float3 color = float3(0, 0, 0);
    
    for (int light = 0; light < LIGHT_COUNT; light++)
    {
        float3 temp = float3(0, 0, 0);
        
        switch (lights[light].lightType)
        {
            case LIGHTTYPE_DIRECTIONAL:
            {
                temp = computeDirectionalLight(data, lights[light], input);
                break;
            }
            case LIGHTTYPE_POINT:
            {
                temp = computePointLight(data, lights[light], input);
                break;
            }
            default:
            {
                temp = float3(0, 0, 0);
                break;
            }
        }

        color = saturate(color + temp);
    }
    
    return float4(color, 1);
}

float4 main(OutputVertex input) : SV_TARGET
{
    PixelData data;
    
    float far_plane = 1000.0;
    float near_plane = 0.1;
    data.depth = (2 * near_plane) / (far_plane + near_plane - t_depth.Sample(diffuseSampler, input.tex).x * (far_plane - near_plane));
    
    data.normal = t_normal.Sample(diffuseSampler, input.tex).xyz;
    data.diffuse = t_diffuse.Sample(diffuseSampler, input.tex);
    data.specular = t_specular.Sample(diffuseSampler, input.tex);
    data.position = t_position.Sample(diffuseSampler, input.tex);
    
    return ComputeLight(data, input);
    
    switch (input.mode)
    {
        case 3:
            return float4(data.depth, data.depth, data.depth, 1);
        case 4:
            return float4(data.diffuse, 1);
        case 5:
            return float4(data.normal, 1);
        case 6:
            return float4(data.specular, 1);
        case 7:
            return float4(data.position, 1);
        default:
            //return computeDirectionalLight(lights[0], input);
            return ComputeLight(data, input);
    }
    
    return float4(data.specular, 1);
    
    return data.depth;
    
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}