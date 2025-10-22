#version 330 core

uniform sampler2D u_bgtexture;
uniform sampler2D u_texture;
uniform sampler2D u_texture2;
uniform vec2 u_solution;
uniform float u_targetDensity;
uniform float u_sampleRadius;
uniform float u_time;

out vec4 fragColor;
vec2 texcoord;

vec4 hsvToRgb(vec4 c);
vec4 lerp(vec4 a, vec4 b, float t);
vec2 getVelocity(vec2 texcoord, vec2 delta);
float getDeltaDensity(vec2 texcoord);
float getDeltaDensitySmoothed(vec2 texcoord, vec2 delta);
vec2 getVelocitySmoothed(vec2 texcoord, vec2 delta);

void main()
{
    texcoord = gl_FragCoord.xy / u_solution;
    float density = texture2D(u_texture, texcoord).r;
    float err = abs(density - u_targetDensity);
    float deltaDensity = getDeltaDensity(texcoord);
    vec2 velocity = getVelocitySmoothed(texcoord, vec2(2.0 / u_solution.x, 2.0 / u_solution.y));

    vec4 startColor = vec4(89 / 255, 179 / 255, 0.9 + deltaDensity, pow(density, 0.5));
    vec4 whiteColor = vec4(255, 255, 255, 255) / 255.0;

    if (err < 0.01) {
        fragColor = lerp(whiteColor, vec4(startColor.xyz, density), err / 0.01);
        return;
    }

    if (density < u_targetDensity - 0.01) {
        fragColor = vec4(0, 0, 0, 0);
        return;
    }

    vec2 dir = length(velocity) == 0.0 ? vec2(0, 0) : normalize(velocity);
    vec2 refractedUV = texcoord - dir * deltaDensity * 0.1;
    startColor = texture(u_bgtexture, refractedUV) + startColor * 0.1;

    fragColor = startColor;
}

vec4 hsvToRgb(vec4 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return vec4(c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y), c.w);
}

vec4 lerp(vec4 a, vec4 b, float t) {
    return a + (b - a) * t;
}

float getDeltaDensity(vec2 texcoord) {
    float d1 = texture2D(u_texture, texcoord).r;
    float d2 = texture2D(u_texture2, texcoord).r;
    return (d1 - d2);
}

vec2 getVelocity(vec2 texcoord, vec2 delta) {
    float dx1 = getDeltaDensitySmoothed(texcoord + vec2(delta.x, 0.0), delta);
    float dx2 = getDeltaDensitySmoothed(texcoord - vec2(delta.x, 0.0), delta);
    float dy1 = getDeltaDensitySmoothed(texcoord + vec2(0.0, delta.y), delta);
    float dy2 = getDeltaDensitySmoothed(texcoord - vec2(0.0, delta.y), delta);
    return -vec2(dx1 - dx2, dy1 - dy2) / delta;
}

vec2 getVelocitySmoothed(vec2 texcoord, vec2 delta) {
    const int radius = 1;
    
    vec2 velocityTotal = vec2(0.0, 0.0);
    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            vec2 offset = vec2(i, j) * delta;
            velocityTotal += getVelocity(texcoord + offset, delta);
        }
    }
    return velocityTotal / pow(float(radius * 2 + 1), 2.0);
}

float getDeltaDensitySmoothed(vec2 texcoord, vec2 delta) {
    const int radius = 3;
    float deltaDensityTotal = 0.0;
    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            vec2 offset = vec2(i, j) * delta;
            deltaDensityTotal += getDeltaDensity(texcoord + offset);
        }
    }
    return deltaDensityTotal / pow(float(radius * 2 + 1), 2.0);
}
