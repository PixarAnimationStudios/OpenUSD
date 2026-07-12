#version 330 core

in vec4 fragColor;
in vec4 fragSelectedColor;
in float fragSelected;
in vec2 fragLocalCoord;
in vec2 fragRectSize;
in float fragInnerStroke;
out vec4 outColor;

uniform float uCornerRadius;
uniform vec4 uInnerStrokeColor;

float roundedRectSDF(vec2 p, vec2 halfSize, float cornerRadius) {
    vec2 q = abs(p) - halfSize + vec2(cornerRadius);
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - cornerRadius;
}

void main() {
    // Blend between normal and selected colors based on selected flag.
    // When fragSelected < 0 (port sentinel: -1.0), use fragColor as fill and
    // fragSelectedColor as per-vertex stroke color instead.
    float selBlend = max(fragSelected, 0.0);
    vec4 baseColor = mix(fragColor, fragSelectedColor, selBlend);

    // Determine stroke color source:
    // - Port vertices (fragSelected < 0): per-vertex color from fragSelectedColor
    // - Node vertices (fragSelected >= 0): uniform uInnerStrokeColor
    bool usePerVertexStroke = fragSelected < 0.0;
    vec4 strokeColor = usePerVertexStroke ? fragSelectedColor : uInnerStrokeColor;

    vec2 halfSize = fragRectSize * 0.5;
    vec2 posRelativeToCenter = fragLocalCoord - halfSize;
    float pixelSizeWorld = length(fwidth(posRelativeToCenter));  // fragcoord or posRelativeToCenter?
    float d = roundedRectSDF(posRelativeToCenter, halfSize, uCornerRadius);
    float aa = pixelSizeWorld * 0.5;
    float fillAlpha = smoothstep(aa, -aa, d);

    // fragInnerStroke encodes three modes:
    //   < 0  stroke-only overlay (center-stroke ring, transparent interior)
    //   == 0 no stroke (bg/title/row quads)
    //   > 0  fill + inner stroke (ports)
    if (fragInnerStroke < 0.0) {
        float halfStroke = abs(fragInnerStroke) * 0.5;
        float ringOuter = smoothstep(halfStroke + aa, halfStroke - aa, d);
        float ringInner = smoothstep(-halfStroke + aa, -halfStroke - aa, d);
        float ringMask = ringOuter - ringInner;
        if (ringMask < 0.001) discard;
        outColor = vec4(strokeColor.rgb, strokeColor.a * ringMask);
        return;
    }

    float outlineAlpha = 1;
    vec3 color = baseColor.rgb;
    if (fragInnerStroke > 0.0) {
        float innerStrokeWorld = max(fragInnerStroke, pixelSizeWorld);
        float innerBoundary = -innerStrokeWorld;
        outlineAlpha = fillAlpha - smoothstep(innerBoundary - aa, innerBoundary + aa, d);
        color = mix(strokeColor.rgb, baseColor.rgb, 1.0 - (1.0 - outlineAlpha) * strokeColor.a);
    }

    outColor = vec4(color, baseColor.a * fillAlpha);
    //outColor = vec4(d * 0.5 + 0.5, d * 0.5 + 0.5, d * 0.5 + 0.5, 1.0);
}
