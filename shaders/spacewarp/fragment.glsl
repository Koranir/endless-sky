// Fragment passthrough shader
precision mediump float;

uniform sampler2D bufferTexture;

in vec2 fragTexCoord;
out vec4 finalColor;

#define pi 3.141592653589793238462643383279
#define fragCoord fragTexCoord
#define iResolution vec2(1, 1)
#define iMouse vec2(1148, 752)
#define fragColor finalColor
#define iChannel0 bufferTexture

void main()
{
    float strength = -0.25f;
    
    vec2 p = 2.0*(fragTexCoord-vec2(0.5)); // map [0, 1] to [-1, 1]
    float theta = atan(p.y, p.x);
    float rd = length(p);
    float ru = rd * (1.0 + strength * rd * rd);
    vec2 uv = vec2(cos(theta), sin(theta)) * ru / 2.0 + 0.5; 
    
    fragColor = texture(bufferTexture, uv);
}