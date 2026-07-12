#version 330 core

in float vAlpha;
in float vSelected;
in float vHovered;
in float vHighlighted;
in float vDir;
out vec4 outColor;

uniform vec3 uLinkColor;
uniform vec3 uSelectedLinkColor;
uniform vec3 uHoveredLinkColor;
uniform vec3 uHighlightedLinkColor;

void main() {
    float d = abs(vDir) - 0.5;
    float pixWidth = length(vec2(dFdx(vDir), dFdy(vDir)));
    float alpha = smoothstep(d-pixWidth, d+pixWidth, 0.0);

    // 4-state blend: normal -> highlighted -> hovered -> selected (selected takes priority)
    vec3 color0 = mix(uLinkColor, uHighlightedLinkColor, vHighlighted);
    float alpha0 = mix(vAlpha, 1.0, vHighlighted);
    vec3 color1 = mix(color0, uHoveredLinkColor, vHovered);
    float alpha1 = mix(alpha0, 1.0, vHovered);
    outColor = mix(vec4(color1, alpha1 * alpha), vec4(uSelectedLinkColor, alpha), vSelected);
}
