#version 330

in vec4 fPosScreen;
in vec3 fPosWorld;
in vec3 fNormal;
in vec2 fTexCoord;

layout(location = 0) out vec4 outDepth;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec4 outTexCoord;

uniform int isMaxDepth;

void main(void) {
    float depth = fPosScreen.z / fPosScreen.w;
    outDepth = vec4(depth, depth, depth, 1.0);
    outPosition = vec4(fPosWorld, 1.0);
    outNormal = vec4(normalize(fNormal) * 0.5 + 0.5, 1.0);
    outTexCoord = vec4(fTexCoord, 1.0, 1.0);
    if (isMaxDepth != 0) {
        gl_FragDepth = 1.0 - depth;
    } else {
        gl_FragDepth = depth;
    }
}
