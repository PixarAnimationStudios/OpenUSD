#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aLocalCoord;
layout(location = 2) in vec2 aRectSize;
layout(location = 3) in vec4 aColor;
layout(location = 4) in float aInnerStroke;
layout(location = 5) in vec4 aSelectedColor;
layout(location = 6) in float aSelected;
layout(location = 7) in float aNodeIndex;

uniform mat4 uProjection;
uniform float uCornerRadius;

// Per-node move offsets, indexed by aNodeIndex (RG32F buffer texture). aNodeIndex
// < 0 means the vertex carries an absolute position and skips the fetch.
uniform samplerBuffer uNodeTransforms;

out vec4 fragColor;
out vec4 fragSelectedColor;
out float fragSelected;
out vec2 fragLocalCoord;
out vec2 fragRectSize;
out float fragInnerStroke;

void main() {
    // Expand each quad outward by a fixed world-space amount so the rounded-
    // corner / stroke SDF in the fragment shader has room to anti-alias.
    //
    // Push a vertex outward ONLY when it sits on the node's actual OUTER edge
    // (aLocalCoord at ~0 or ~aRectSize). Interior sub-quad edges — every row
    // stripe, the row spacers, and the title bar's bottom edge — reference the
    // full node rect, so the previous `aLocalCoord < 0.5 * aRectSize` midpoint
    // test pushed BOTH of a sub-quad's edges the same way and TRANSLATED the
    // whole sub-quad by `expand`: stripes above the node's vertical midpoint
    // shifted up, stripes below shifted down, while the (unexpanded) MSDF text
    // stayed put. That is the row-stripe-vs-text drift on tall nodes. Expanding
    // only at the true node boundary leaves interior quads exactly where layout
    // placed them, so stripes stay locked to their text.
    float expand = max(uCornerRadius, 4.0);

    vec3 pos = aPosition;
    // Apply the per-node move offset to the quad origin before expansion so
    // translation never alters the rounded-corner/stroke geometry.
    if (aNodeIndex >= 0.0) {
        vec2 offset = texelFetch(uNodeTransforms, int(aNodeIndex)).rg;
        pos.xy += offset;
    }

    // Outward direction per axis: -1 at the min edge, +1 at the max edge, 0 for
    // interior edges (no displacement). The 0.5 epsilon absorbs float noise;
    // interior stripe edges are inset by the node margins, well clear of it.
    vec2 dir = vec2(0.0);
    if (aLocalCoord.x <= 0.5) {
        dir.x = -1.0;
    } else if (aLocalCoord.x >= aRectSize.x - 0.5) {
        dir.x = 1.0;
    }
    if (aLocalCoord.y <= 0.5) {
        dir.y = -1.0;
    } else if (aLocalCoord.y >= aRectSize.y - 0.5) {
        dir.y = 1.0;
    }
    pos.xy += expand * dir;
    gl_Position = uProjection * vec4(pos, 1.0);

    fragColor = aColor;
    fragSelectedColor = aSelectedColor;
    fragSelected = aSelected;
    fragRectSize = aRectSize;
    fragInnerStroke = aInnerStroke;

    // The frag localCoord follows the vertex: a vertex pushed out by expand*dir
    // sees its logical coord shift by the same amount, so the SDF (measured
    // against aRectSize) still sees the true rect edges. Interior vertices
    // (dir == 0) keep their exact layout coord.
    fragLocalCoord = aLocalCoord + expand * dir;
}
