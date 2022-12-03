// vertex starfield shader
uniform mat2 rotate;
uniform vec2 translate;
uniform vec2 scale;
uniform float elongation;
uniform float brightness;
uniform int parallax;

in vec2 offset;
in float size;
in float corner;
in float depth;
out float fragmentAlpha;
out vec2 coord;
out float depthDist;

void main()
{
	float parallaxMod = (1 + (1 - depth)*parallax);
	fragmentAlpha = brightness * parallaxMod * (4. / (4. + elongation)) * size * .2 + .05;
	coord = vec2(sin(corner), cos(corner));
	depthDist = depth;
	vec2 elongated = vec2(coord.x * size, coord.y * (size + elongation));
	vec2 pos = translate + offset;
	gl_Position = vec4((rotate * elongated * (parallax * 2) + pos * (parallaxMod)) * scale, 0, 1);
}
