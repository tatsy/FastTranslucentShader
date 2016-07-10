#version 330

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;

out vec4 fPosScreen;
out vec3 fPosCamera;
out vec3 fNrmCamera;
out vec2 fTexCoord;
out vec3 fLightPos;

uniform mat4 uMVPMat;
uniform mat4 uMVMat;
uniform vec3 uLightPos;

void main(void) {
    gl_Position = uMVPMat * vec4(vPosition, 1.0);
    fPosScreen = gl_Position;

    fPosCamera = (uMVMat * vec4(vPosition, 1.0)).xyz;
    fNrmCamera = (transpose(inverse(uMVMat)) * vec4(vNormal, 0.0)).xyz;
    fTexCoord  = vTexCoord;
    fLightPos  = (uMVMat * vec4(uLightPos, 1.0)).xyz;
}
