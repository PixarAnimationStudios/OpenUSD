#version 330 core

in vec2 vTexCoord;
out vec4 FragColor;

uniform float uPxRange;
uniform sampler2D uAtlas;
uniform vec4 uTextColor;

float screenPxRange() {
    vec2 unitRange = vec2(uPxRange) / vec2(textureSize(uAtlas, 0));
    vec2 screenTexSize = vec2(1.0) / fwidth(vTexCoord);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    // Sample the SDF texture
    vec3 msd = texture(uAtlas, vTexCoord).rgb;
    float sd = median(msd.r, msd.g, msd.b);

    // Compute alpha value
    float extraThickness = 0; //0.003; // to account for very thin lines getting lost, a slight extra boost in 'font weight'
    float screenPxDistance = screenPxRange()*(sd - 0.5 + extraThickness);
    float alpha = clamp(screenPxDistance + 0.5, 0, 1);
    alpha = pow(alpha, 1.0 / 2.2);

    FragColor = vec4(uTextColor.rgb, alpha * uTextColor.a);
}
