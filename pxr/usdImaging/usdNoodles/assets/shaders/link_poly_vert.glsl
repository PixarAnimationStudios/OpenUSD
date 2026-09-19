#version 330 core

// NOTE: The link-curve shaping in main() below (the cubicBezier / bezier6 / mx
// forward<->backward blend and the verticalEndTangent prim-target cubic, plus
// their numeric constants) is mirrored on the CPU for hit-testing in
// LinkGeometry::sampleLinkCurve, with the constants centralized in
// noodles/render/LinkCurveParams.h. There is no compile-time link between the
// two. If you change the shaping math or a constant here, update those too (and
// the golden tests in tests/noodles/NoodlesRenderTest.cpp) or hit-testing will
// silently drift from the rendered curve.

layout(location = 0) in vec2 refPosition;
layout(location = 1) in vec2 prevPosition;
layout(location = 2) in vec2 nextPosition;
layout(location = 3) in float dir;
layout(location = 4) in vec2 startPoint;
layout(location = 5) in vec2 endPoint;
layout(location = 6) in float selected;
layout(location = 7) in float hovered;
layout(location = 8) in float verticalEndTangent;
layout(location = 9) in float highlighted;
layout(location = 10) in float isPrimTarget;

uniform mat4 uProjection;
uniform float uThickness;
uniform float uZoom;

// Dimming uniforms (calculate alpha in shader instead of CPU)
uniform vec2 uViewportCenter;
uniform float uMaxViewportDim;
uniform float uDimmingStart;
uniform float uDimmingEnd;
uniform float uCutoffAlpha;
uniform float uDimming;
uniform float uAlphaBase;

out float vAlpha;
out float vSelected;
out float vHovered;
out float vHighlighted;
out float vDir;

float doubleFreqSinSteep(float x, float k) {
    const float PI = 3.1415926535897932384626433832795;

    // Base oscillation
    float base = sin(PI * x) * cos(PI * x) * 2.0;

    // Amplitude modulation for steepness control
    float mod = pow(abs(sin(2.0 * PI * x)), k);

    return base * mod;
}

vec2 cubicBezier(vec2 p0, vec2 p1, vec2 p2, vec2 p3, float t) {
    float u = 1.0 - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    vec2 p = uuu * p0;
    p += 3.0 * uu * t * p1;
    p += 3.0 * u * tt * p2;
    p += ttt * p3;

    return p;
}

vec2 bezier6(vec2 p0, vec2 p1, vec2 p2, vec2 p3, vec2 p4, vec2 p5, float t) {
    vec2 mid = (p0 + p5) * 0.5;
    if (t < 0.5) {
        return cubicBezier(p0, p1, p2, mid, t * 2.0);
    } else {
        return cubicBezier(mid, p3, p4, p5, (t - 0.5) * 2.0);
    }
}

void main() {
    // Extra padding to allow extra fragments in small thicknesses in order to properly antialias
    // at the cost of a slight bit of overdraw.
    float padding = 0.5 / uZoom;
    vec2 rP = refPosition;
    vec2 pP = prevPosition;
    vec2 nP = nextPosition;
    // Prim-target relationship links inset the end up by arrow_length so the
    // ribbon stops at the arrowhead base; the arrowhead (drawn separately) fills
    // the gap to the node top. Matches the CPU arrow_length = max(10/zoom, 2).
    float arrowInset = (isPrimTarget > 0.5) ? max(10.0 / uZoom, 2.0) : 0.0;
    vec2 endP = vec2(endPoint.x, endPoint.y - arrowInset);
    vec2 scale = endP - startPoint;
    vec2 position = startPoint + rP * scale;
    vec2 prevFinalPosition = startPoint + pP * scale;
    vec2 nextFinalPosition = startPoint + nP * scale;
    // Another experimental backwards link hack, eval a bezier here by using the forward link
    // profile curve as a parameter. Whole-prim targets override this with a cubic that enters
    // the top-center anchor vertically so the noodle flows into the triangle smoothly.
    {
        if (verticalEndTangent > 0.5) {
            float horizontalHandle = clamp(abs(scale.x) * 0.35, 20.0, 120.0);
            float horizontalDir = abs(scale.x) > 0.0001 ? sign(scale.x) : 1.0;
            float verticalHandle = clamp(abs(scale.y) * 0.35, 20.0, 120.0);
            vec2 p0 = startPoint;
            vec2 p1 = vec2(startPoint.x + horizontalDir * horizontalHandle, startPoint.y);
            vec2 p2 = vec2(endP.x, endP.y - verticalHandle);
            vec2 p3 = endP;
            position = cubicBezier(p0, p1, p2, p3, rP.x);
            prevFinalPosition = cubicBezier(p0, p1, p2, p3, pP.x);
            nextFinalPosition = cubicBezier(p0, p1, p2, p3, nP.x);
        } else {
            // transition between forward and reverse link, in world units
            const float tFR = 500.0;
            const float tOffset = 50.0;    // start the transition before zero crossing
            const float yMinDist = 200.0;   // minimum y-distance if endpoints are too close: to use simple curve
            float d = clamp((startPoint.x - endP.x) * 0.1, 100.0, tFR);
            float midSharpness = 0.75;
            //float midSharpness = 1.0 - clamp(length(scale) / 1000.0, 0.05, 0.3);
            vec2 m = (startPoint + endP) * 0.5;
            float sgn = sign(endP.y - startPoint.y);
            float p2yOff = max((m.y - startPoint.y) * midSharpness, sgn*100.0);
            float p3yOff = max((endP.y - m.y) * midSharpness, sgn*100.0);
            vec2 p0 = startPoint;
            vec2 p1 = vec2(startPoint.x + d, startPoint.y);
            vec2 p2 = vec2(startPoint.x + d, startPoint.y + p2yOff);
            vec2 p3 = vec2(endP.x - d, endP.y - p3yOff);
            vec2 p4 = vec2(endP.x - d, endP.y);
            vec2 p5 = endP;
            float tOffsetFactor = clamp(abs(startPoint.y - endP.y) / yMinDist, 0.0, 1.0);  // if y is close, adjust offset
            float mx = clamp((startPoint.x - endP.x + (tFR + tOffset) * tOffsetFactor) * 0.5, 0.0, tFR) / tFR;
            mx = smoothstep(0.0, 1.0, mx);  // smooth the transition
            position = mix(position, bezier6(p0, p1, p2, p3, p4, p5, rP.x), mx);
            prevFinalPosition = mix(prevFinalPosition, bezier6(p0, p1, p2, p3, p4, p5, pP.x), mx);
            nextFinalPosition = mix(nextFinalPosition, bezier6(p0, p1, p2, p3, p4, p5, nP.x), mx);
        }
    }
    vec2 tan = normalize(nextFinalPosition - prevFinalPosition);
    vec2 norm = vec2(-tan.y, tan.x);
    vec2 finalPosition = position + (uThickness + padding) * norm;
    gl_Position = uProjection * vec4(finalPosition, 0.0, 1.0);

    // Calculate alpha in shader (GPU) instead of CPU
    // Distance from link endpoints to viewport center
    vec2 startToCenter = startPoint - uViewportCenter;
    vec2 endToCenter = endP - uViewportCenter;
    float startDistWorld = length(startToCenter);
    float endDistWorld = length(endToCenter);
    float linkDistWorld = min(startDistWorld, endDistWorld);

    // Dimming thresholds in world space
    float linkCutoffEndWorld = uMaxViewportDim * uDimmingEnd;
    float linkCutoffStartWorld = uMaxViewportDim * uDimmingStart;
    float spreadWorld = abs(linkCutoffStartWorld - linkCutoffEndWorld);

    // Calculate dimming value (with divide-by-zero protection)
    float minLinkDist = linkDistWorld - linkCutoffEndWorld;
    float val = spreadWorld > 0.0 ? clamp(minLinkDist / spreadWorld, 0.0, uCutoffAlpha) : 0.0;
    val = uDimming * val;

    // Final alpha
    vAlpha = uAlphaBase * (1.0 - val);
    vSelected = selected;
    vHovered = hovered;
    vHighlighted = highlighted;
    // Don't bake the AA padding into vDir — the fragment shader's edge test at
    // abs(vDir)=0.5 must sit at the same fraction of the ribbon regardless of
    // zoom, so the visible link width is zoom-independent in world space.
    vDir = dir > 0.0 ? 1.0 : -1.0;
}
