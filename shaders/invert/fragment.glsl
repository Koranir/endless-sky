// Fragment passthrough shader
precision mediump float;

uniform sampler2D bufferTexture;

in vec2 fragTexCoord;
out vec4 finalColor;

void main()
{
	finalColor = vec4(vec3(1.0) - (texture(bufferTexture, fragTexCoord).rgb), 1.0);
}