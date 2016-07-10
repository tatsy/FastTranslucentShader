#version 330

in vec3 fPosWorld;
in vec4 fPosScreen;
in vec3 fNrmWorld;
in vec2 fTexCoord;
in float fRadius;

out vec4 outColor;

uniform sampler2D uPositionMap;
uniform sampler2D uNormalMap;
uniform sampler2D uTexCoordMap;

uniform vec3 uLightPos;

float Pi = 4.0 * atan(1.0);

uniform float eta;
uniform vec3 sigma_a;
uniform vec3 sigmap_s;

float Fdr() {
    if (eta >= 1.0) {
        return -1.4399 / (eta * eta) + 0.7099 / eta + 0.6681 + 0.0636 * eta;
    } else {
        return -0.4399 + 0.7099 / eta - 0.3319 / (eta * eta) + 0.0636 / (eta * eta * eta);
    }
}

vec3 diffRef(vec3 p0, vec3 p1) {
    float A = (1.0 + Fdr()) / (1.0 - Fdr());
    vec3 sigmapt  = sigma_a + sigmap_s;
    vec3 sigma_tr = sqrt(3.0 * sigma_a * sigmapt);
    vec3 alphap   = sigmap_s / sigmapt;
    vec3 zpos     = 1.0 / sigmapt;
    vec3 zneg     = zpos * (1.0 + (4.0 / 3.0) * A);

    float dist = distance(p0, p1);
    float d2 = dist * dist;

    vec3 dpos = sqrt(d2 + zpos * zpos);
    vec3 dneg = sqrt(d2 + zneg * zneg);
    vec3 posterm = zpos * (dpos * sigma_tr + 1.0) * exp(-sigma_tr * dpos) / (dpos * dpos * dpos);
    vec3 negterm = zneg * (dneg * sigma_tr + 1.0) * exp(-sigma_tr * dneg) / (dneg * dneg * dneg);
    vec3 rd = (alphap / (4.0 * Pi)) * (posterm + negterm);
    return max(vec3(0.0, 0.0, 0.0), rd);
}

void main(void) {
    vec2 texCoord = fPosScreen.xy / fPosScreen.w * 0.5 + 0.5;
    vec3 pos = texture(uPositionMap, texCoord).xyz;

    vec3 L = normalize(uLightPos - fPosWorld);
    vec3 N = normalize(fNrmWorld);
    float E = max(0.0, dot(N, L));

    float dA = fRadius * fRadius * Pi * 0.001;
    vec3  Mo = diffRef(pos, fPosWorld) * E * dA;

    outColor = vec4(Mo, 1.0);
}
