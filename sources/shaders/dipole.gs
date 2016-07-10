#version 330
#extension GL_EXT_geometry_shader4 : enable

layout(points) in;
layout(triangle_strip, max_vertices=12) out;

in vec3  gPosition[];
in vec3  gNormal[];
in vec2  gTexCoord[];
in float gRadius[];

out vec3 fPosWorld;
out vec4 fPosScreen;
out vec3 fNrmWorld;
out vec2 fTexCoord;
out float fRadius;

uniform mat4 uMVPMat;
uniform mat4 uMVMat;

void processVertex(vec3 pos) {
    gl_Position = uMVPMat * vec4(pos, 1.0);
    fPosScreen = gl_Position;
    fPosWorld  = gPosition[0];
    EmitVertex();
}

void main(void) {
    vec3 uAxis, vAxis, wAxis;
    wAxis = normalize(gNormal[0]);
    if (abs(wAxis.y) < 0.1) {
        vAxis = vec3(0.0, 1.0, 0.0);
    } else {
        vAxis = vec3(1.0, 0.0, 0.0);
    }
    uAxis = cross(vAxis, wAxis);
    vAxis = cross(uAxis, wAxis);

    float r = gRadius[0] * 0.1;
    vec3 p00 = gPosition[0] - uAxis * r - vAxis * r;
    vec3 p01 = gPosition[0] - uAxis * r + vAxis * r;
    vec3 p10 = gPosition[0] + uAxis * r - vAxis * r;
    vec3 p11 = gPosition[0] + uAxis * r + vAxis * r;

    fTexCoord  = gTexCoord[0];
    fNrmWorld = gNormal[0];
    fRadius = gRadius[0] * 0.1;

    processVertex(p00);
    processVertex(p01);
    processVertex(p10);
    processVertex(p11);
    EndPrimitive();
}
