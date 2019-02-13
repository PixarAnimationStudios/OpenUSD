// Check whether we're in a GLSL context. If so, we don't expect "floatN"
// types to exist, so we use "vecN" types instead.
#if defined(__VERSION__) && (__VERSION__ >= 110)
#define float3 vec3
#endif

mayaSurfaceShaderOutput
usdPreviewSurfaceCombiner(
        float3 diffuseIrradianceIn,
        float3 specularIrradianceIn,
        float3 ambientIn,
        float3 irradianceEnv,
        float3 specularEnv,
        float3 diffuseColor,
        float3 specularColor,
        float3 emissiveColor,
        float3 transparency)
{
    mayaSurfaceShaderOutput result;

    // Ambient
    result.outColor = ambientIn * diffuseColor;

    // Diffuse
    result.outColor += diffuseIrradianceIn;
    result.outColor += diffuseColor * irradianceEnv;

    if (!mayaAlphaCut) {
        result.outColor *= saturate(1.0 - transparency); 
    }

    // Specular
    result.outColor += specularIrradianceIn;
    result.outColor += specularColor * specularEnv;

    // Emissive
    result.outColor += emissiveColor;

    // Transparency
    result.outTransparency = transparency;

    result.outGlowColor = float3(0.0, 0.0, 0.0);
    result.outMatteOpacity = (1.0 - transparency);

    return result;
}
