// Fragment passthrough shader
precision mediump float;

uniform sampler2D bufferTexture;

in vec2 fragTexCoord;
out vec4 finalColor;

void main()
{
	const float radius1 = 4.0;
	const float radius2 = 4.0;
	const float HDRPower = 2.0;
	const vec2 iResolution = 1.0/vec2(2256, 1504);

	vec3 color = texture(bufferTexture, fragTexCoord).rgb;

	vec3 bloom_sum1 = texture(bufferTexture, fragTexCoord + vec2(1.5, -1.5) * radius1 * iResolution).rgb;
	bloom_sum1 += texture(bufferTexture, fragTexCoord + vec2(-1.5, -1.5) * radius1 * iResolution).rgb;
	bloom_sum1 += texture(bufferTexture, fragTexCoord + vec2( 1.5,  1.5) * radius1 * iResolution).rgb;
	bloom_sum1 += texture(bufferTexture, fragTexCoord + vec2(-1.5,  1.5) * radius1 * iResolution).rgb;
	bloom_sum1 += texture(bufferTexture, fragTexCoord + vec2( 0.0, -2.5) * radius1 * iResolution).rgb;
	bloom_sum1 += texture(bufferTexture, fragTexCoord + vec2( 0.0,  2.5) * radius1 * iResolution).rgb;
	bloom_sum1 += texture(bufferTexture, fragTexCoord + vec2(-2.5,  0.0) * radius1 * iResolution).rgb;
	bloom_sum1 += texture(bufferTexture, fragTexCoord + vec2( 2.5,  0.0) * radius1 * iResolution).rgb;

	bloom_sum1 *= 0.005;

	vec3 bloom_sum2 = texture(bufferTexture, fragTexCoord + vec2(1.5, -1.5) * radius2 * iResolution).rgb;
	bloom_sum2 += texture(bufferTexture, fragTexCoord + vec2(-1.5, -1.5) * radius2 * iResolution).rgb;
	bloom_sum2 += texture(bufferTexture, fragTexCoord + vec2( 1.5,  1.5) * radius2 * iResolution).rgb;
	bloom_sum2 += texture(bufferTexture, fragTexCoord + vec2(-1.5,  1.5) * radius2 * iResolution).rgb;
	bloom_sum2 += texture(bufferTexture, fragTexCoord + vec2( 0.0, -2.5) * radius2 * iResolution).rgb;	
	bloom_sum2 += texture(bufferTexture, fragTexCoord + vec2( 0.0,  2.5) * radius2 * iResolution).rgb;
	bloom_sum2 += texture(bufferTexture, fragTexCoord + vec2(-2.5,  0.0) * radius2 * iResolution).rgb;
	bloom_sum2 += texture(bufferTexture, fragTexCoord + vec2( 2.5,  0.0) * radius2 * iResolution).rgb;

	bloom_sum2 *= 0.010;

	float dist = radius2 - radius1;
	vec3 HDR = (color + (bloom_sum2 - bloom_sum1)) * dist;
	vec3 blend = HDR + color;
	color = pow(vec3(abs(blend)), vec3(abs(HDRPower))) + HDR; // pow - don't use fractions for HDRpower
	
	finalColor = vec4(clamp(color, 0.0, 1.0), 1);
}