// Fragment passthrough shader
precision mediump float;

uniform sampler2D bufferTexture;
uniform float accredition;
uniform float radius;
uniform vec2 center;
uniform vec2 position;
uniform float zoom;
uniform vec2 resolution;

in vec2 fragTexCoord;
out vec4 finalColor;

#define pi 3.141592653589793238462643383279
#define iResolution resolution
#define fragCoord fragTexCoord*iResolution
#define fragColor finalColor
#define iChannel0 bufferTexture
#define iMouse ((center-position) * zoom * vec2(-1, 1) + 0.5*iResolution)

#define pi 3.141592653589793238462643383279

void main()
{
	
	vec2 uv = fragCoord.xy / iResolution.xy;
	vec2 mouse = iMouse.xy / iResolution.xy;
	vec2 warp = normalize(iMouse.xy - fragCoord.xy) * pow(length((iMouse.xy - fragCoord.xy) / radius / zoom), -2.0) * 20.0;
//	warp.y = -warp.y;
	uv = uv + warp;
	
	float light = clamp(distance(iMouse.xy, fragCoord.xy)/accredition/zoom - 30, 0.0, 1.0);

	vec4 color = texture(iChannel0, uv);
	fragColor = color * light;
	

}