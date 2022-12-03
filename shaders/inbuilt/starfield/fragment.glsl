// fragment starfield shader
precision mediump float;
in float fragmentAlpha;
in vec2 coord;
in float depthDist;
out vec4 finalColor;

void main()
{
	float alpha = fragmentAlpha * (1. - abs(coord.x) - abs(coord.y));
	vec3 color = mix(vec3(0.8, 0.8, 1), vec3(1, 0.5, 0.5), depthDist);
	finalColor = vec4(color, 1) * alpha;
}
