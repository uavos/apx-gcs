// Based on http://kodemongki.blogspot.com/2011/06/kameraku-custom-shader-effects-example.html

uniform sampler2D source;
uniform lowp float qt_Opacity;

uniform float blurSize;

varying vec2 qt_TexCoord0;


void main()
{
    vec2 uv = qt_TexCoord0.xy;
    /*//blur
    vec4 c = vec4(0.0);
    c += texture2D(source, uv - vec2(4.0*blurSize, 0.0)) * 0.05;
    c += texture2D(source, uv - vec2(3.0*blurSize, 0.0)) * 0.09;
    c += texture2D(source, uv - vec2(2.0*blurSize, 0.0)) * 0.12;
    c += texture2D(source, uv - vec2(1.0*blurSize, 0.0)) * 0.15;
    c += texture2D(source, uv) * 0.18;
    c += texture2D(source, uv + vec2(1.0*blurSize, 0.0)) * 0.15;
    c += texture2D(source, uv + vec2(2.0*blurSize, 0.0)) * 0.12;
    c += texture2D(source, uv + vec2(3.0*blurSize, 0.0)) * 0.09;
    c += texture2D(source, uv + vec2(4.0*blurSize, 0.0)) * 0.05;
*/

    vec4 orig = texture2D(source, uv);
    float cr = pow(0.05, 2.0);
    float pt = pow(uv.x - 0.5, 2.0) + pow(uv.y - 0.5, 2.0);
    float d = pt - cr;
    float cf = 1.0;
    if (d > 0.0)
        cf = 1.0 - 2.0 * d;
    vec3 col = cf * orig.rgb;
    gl_FragColor = qt_Opacity * vec4(col, 1.0);
}

