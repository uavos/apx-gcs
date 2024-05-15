#version 440

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
} ubuf;

layout(binding = 1) uniform sampler2D source;

void main()
{
    vec2 uv = texCoord.xy;
    vec4 orig = texture(source, uv);
    float cr = pow(0.05, 2.0);
    float pt = pow(uv.x - 0.5, 2.0) + pow(uv.y - 0.5, 2.0);
    float d = pt - cr;
    float cf = 1.0;
    if (d > 0.0)
        cf = 1.0 - 2.0 * d;
    vec3 col = cf * orig.rgb;
    fragColor = ubuf.qt_Opacity * vec4(col, 1.0);
}
