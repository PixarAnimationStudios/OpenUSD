// Check whether we're in a GLSL context. If so, we don't expect "floatN"
// types to exist, so we use "vecN" types instead.
#if defined(__VERSION__) && (__VERSION__ >= 110)
#define float4 vec4
#endif

float
float4ToFloatY(float4 vectorInput)
{
    return vectorInput.y;
}
