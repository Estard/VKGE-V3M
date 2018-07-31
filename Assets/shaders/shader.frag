#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set= 1,binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragLight;
layout(location = 4) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

layout(location = 5) in fragTexCoords
{
	vec2 fragTCxy;
	vec2 fragTCzx;
	vec2 fragTCyz;
	float bx;
	float by;
	float bz;
	bool multiTexture;
}texCoords;

/*layout(push_constant) uniform PushConstants
 {

 vec4 ambient;
 vec4 diffuse;
 vec4 specular;
 float opacity;
 bool multiTexture;
 }material;
 */
void main()
{
	vec4 baseColor;

	if (texCoords.bx < 1.1)
	{
		vec2 xy = vec2(mod(texCoords.fragTCxy.x, 0.5),
				mod(texCoords.fragTCxy.y, 0.5));
		if (texCoords.fragTCzx.x < 0.75)
		{
			xy.x = xy.x + 0.5;
		}

		vec2 zx = vec2(mod(texCoords.fragTCzx.x, 0.5) + 0.5,
				mod(texCoords.fragTCzx.y, 0.5) + 0.5);

		vec2 yz = vec2(mod(texCoords.fragTCyz.x, 0.5) + 0.5,
				mod(texCoords.fragTCyz.y, 0.5) + 0.5);

		baseColor = vec4(
				texture(texSampler, xy) * texCoords.bz
						+ texture(texSampler, zx) * texCoords.by
						+ texture(texSampler, yz) * texCoords.bx);
	}
	else
	{
		baseColor = texture(texSampler, fragTexCoord) * vec4(fragColor, 1.0);
	}

	vec3 L = normalize(fragLight);
	vec3 N = normalize(fragNormal);
	vec3 V = -fragPos;

	vec3 H = normalize(L + V);
	vec3 reflected = reflect(-L, N);
	float RdotV = max(dot(H, N), 0.0);
	float spec = pow(RdotV, 1);

	float ambient = 0.01;
	float diffuse = max(dot(N, L), 0.0) ;
	float specular = spec /4.0;
	outColor = (ambient + diffuse + specular) * baseColor;
}

