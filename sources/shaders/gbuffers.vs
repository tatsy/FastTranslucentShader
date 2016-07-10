#version 330

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;

out vec4 fPosScreen;
out vec3 fPosWorld;
out vec3 fNormal;
out vec2 fTexCoord;

uniform mat4 uMVPMat;

void main(void) {
    gl_Position = uMVPMat * vec4(vPosition, 1.0);
    fPosScreen  = gl_Position;
    fPosWorld   = vPosition;
    fNormal     = vNormal;
    fTexCoord   = vTexCoord;
}
