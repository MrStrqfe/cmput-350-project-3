#version 120

// Lights the cave: each light is a warm pool that fades to a white core,
// with a slight flicker. Far from every light only 'ambient' is left. When
// oil runs low, the dark parts lose their colour and turn grey-blue.
// Uniforms are set by LightSystem every frame.

uniform sampler2D texture;
uniform vec4 lights[16];   // xy = position in gl_FragCoord pixels, z = radius in pixels, w = flicker
uniform int lightCount;    // how many of lights[] are used
uniform float ambient;     // brightness far from any light, 0 to 1
uniform float time;        // seconds, drives the flicker
uniform float oil;         // lantern fuel, 0 (empty) to 1 (full)

void main() {
    vec4 color = texture2D(texture, gl_TexCoord[0].xy) * gl_Color;

    // Brightness here: the brightest light wins.
    float bright = ambient;
    for (int i = 0; i < 16; ++i) {
        if (i >= lightCount) {
            break;
        }
        vec4 light = lights[i];
        float radius = light.z * (1.0 + light.w * sin(time * 9.0 + float(i) * 1.7));
        bright = max(bright, smoothstep(radius, 0.35 * radius, distance(gl_FragCoord.xy, light.xy)));
    }

    // Dim light is orange, full light keeps the sprite's own colours.
    vec3 lit = color.rgb * bright * mix(vec3(1.0, 0.75, 0.45), vec3(1.0), bright);

    // Low oil: below 40 % the darkness drains to grey-blue (fully at 10 %).
    float grey = dot(lit, vec3(0.299, 0.587, 0.114));
    vec3 cold = vec3(grey * 0.8, grey * 0.9, grey * 1.3);
    float drain = (1.0 - smoothstep(0.1, 0.4, oil)) * (1.0 - bright);

    gl_FragColor = vec4(mix(lit, cold, drain), color.a);
}
