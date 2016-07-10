#version 330

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;
layout(location = 3) in float vRadius;

out vec3  gPosition;
out vec3  gNormal;
out vec2  gTexCoord;
out float gRadius;

void main(void) {
    gPosition = vPosition;
    gNormal   = vNormal;
    gTexCoord = vTexCoord;
    gRadius   = vRadius;
}
