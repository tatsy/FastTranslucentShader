#version 330

in vec4 fPosScreen;
in vec3 fPosCamera;
in vec3 fNrmCamera;
in vec2 fTexCoord;
in vec3 fLightPos;

out vec4 outColor;

uniform sampler2D uTransMap;

uniform float refFactor;
uniform float transFactor;

vec2 reflectRatio(vec3 V, vec3 N) {
    float etaO = 1.0;
    float etaI = 1.3;
    float nnt = etaO / etaI;
    float ddn = dot(-V, N);
    float cos2t = 1.0 - nnt * nnt * (1.0 - ddn * ddn);
    if (cos2t < 0.0) {
        return vec2(1.0, 0.0);
    }

    float a = etaO - etaI;
    float b = etaO + etaI;
    float R0 = (a * a) / (b * b);
    float c = 1.0 + ddn;
    float Re = R0 + (1.0 - R0) * pow(c, 5.0);
    float nnt2 = pow(etaI / etaO, 2.0);
    float Tr = (1.0 - Re) * nnt2;
    return vec2(Re, Tr);
}

void main(void) {
    vec3 V = normalize(-fPosCamera);
    vec3 N = normalize(fNrmCamera);
    vec3 L = normalize(fLightPos - fPosCamera);
    vec3 H = normalize(V + L);

    vec2 texCoord = fPosScreen.xy / fPosScreen.w * 0.5 + 0.5;
    vec3 trans = texture(uTransMap, texCoord).xyz;

    float NdotL = max(0.0, dot(N, L));
    float NdotH = max(0.0, dot(N, H));

    vec3 diffuse = vec3(1.0, 1.0, 1.0) * NdotL;
    vec3 specular = vec3(1.0, 1.0, 1.0) * pow(NdotH, 128.0) * 0.2;
    vec2 Re = vec2(0.5, 0.5);

    vec3 rgb = transFactor * Re.y * trans +
               refFactor   * Re.x * (diffuse + specular);
    outColor = vec4(rgb, 1.0);
}
