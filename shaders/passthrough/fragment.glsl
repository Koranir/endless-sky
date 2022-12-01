// Fragment passthrough shader
precision mediump float;

uniform sampler2D bufferTexture;

in vec2 fragTexCoord;
out vec4 finalColor;

void main()
{
	finalColor = texture(bufferTexture, fragTexCoord);
}