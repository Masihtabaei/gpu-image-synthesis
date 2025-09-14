struct VertexShaderOutput
{
    float4 clipSpacePosition : SV_POSITION;
    float3 viewSpacePosition : POSITION;
    float3 viewSpaceNormal : NORMAL;
    float2 texCoord : TEXCOOD;
};

/// <summary>
/// Constants that can change every frame.
/// </summary>
cbuffer PerFrameConstants : register(b0)
{
    float4x4 projectionMatrix;
    float3 boundingBoxColor;
    bool useNormalMapping;
    float lightDirectionXCoordinate;
    float lightDirectionYCoordinate;
    float lightIntensityFactor;
}

/// <summary>
/// Constants that can change per Mesh/Draw call.
/// </summary>
cbuffer PerMeshConstants : register(b1)
{
    float4x4 modelViewMatrix;

}

/// <summary>
/// Constants that are really constant for the entire scene.
/// </summary>
cbuffer Material : register(b2)
{
    float4 emissiveMaterialParameters;
    float4 ambientMaterialParameters;
    float4 diffuseMaterialParameters;
    float4 specularMaterialParameters;
}


/// <summary>
/// Constants that can change per Mesh/Draw call.
/// </summary>
cbuffer AABBConstants : register(b3)
{
    float4 lower;
    float4 upper;
}

Texture2D<float4> g_textureAmbient : register(t0);
Texture2D<float4> g_textureDiffuse : register(t1);
Texture2D<float4> g_textureSpecular : register(t2);
Texture2D<float4> g_textureEmissive : register(t3);
Texture2D<float4> g_textureNormal : register(t4);

SamplerState g_sampler : register(s0);

VertexShaderOutput VS_main(float3 position : POSITION, float3 normal : NORMAL, float2 texCoord : TEXCOORD)
{
    VertexShaderOutput output;

    float4 p4 = mul(modelViewMatrix, float4(position, 1.0f));
    output.viewSpacePosition = p4.xyz;
    output.viewSpaceNormal = mul(modelViewMatrix, float4(normal, 0.0f));
    output.clipSpacePosition = mul(projectionMatrix, p4);
    output.texCoord = texCoord;
    return output;
}

float4 PS_main(VertexShaderOutput input)
    : SV_TARGET
{
    float3 lightIntensity = float3(lightIntensityFactor, lightIntensityFactor, lightIntensityFactor);
    
    float3 sampledDiffuseColor = g_textureDiffuse.Sample(g_sampler, input.texCoord, 0).rgb;
    float3 sampledAmbientColor = g_textureAmbient.Sample(g_sampler, input.texCoord, 0).rgb;
    float3 sampledSpecularColor = g_textureSpecular.Sample(g_sampler, input.texCoord, 0).rgb;
    float3 sampledEmissiveColor = g_textureEmissive.Sample(g_sampler, input.texCoord, 0).rgb;
    float3 sampledNormalMapTexture = g_textureNormal.Sample(g_sampler, input.texCoord, 0).rgb;
    


    float3 lightDirection = float3(lightDirectionXCoordinate, lightDirectionYCoordinate, -1.0f);
    float3 normalizeLightDirectionVector = normalize(lightDirection);
    float3 normalizedViewVector = normalize(-input.viewSpacePosition);
    
    float3 normalizedNormalVector = normalize(input.viewSpaceNormal);
    
    float3 normalizedHalfwayVector = normalize(normalizeLightDirectionVector + normalizedViewVector);
    float diffuslyReflectedLight = max(0.0f, dot(normalizedNormalVector, normalizeLightDirectionVector));
    float specularReflectedLight = pow(max(0.0f, dot(normalizedNormalVector, normalizedHalfwayVector)), specularMaterialParameters.w);
    
    float3 finalColor = emissiveMaterialParameters.xyz * sampledEmissiveColor
                        + 
                        ambientMaterialParameters.xyz * sampledAmbientColor
                        + 
                        lightIntensity * diffuslyReflectedLight * diffuseMaterialParameters.xyz * sampledDiffuseColor.xyz
                        +
                        lightIntensity * specularReflectedLight * specularMaterialParameters.xyz * sampledSpecularColor.xyz;
    

    return float4(finalColor, 1.0f);
}