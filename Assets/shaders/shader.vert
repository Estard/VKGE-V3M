#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferData
{
	mat4 model;
	mat4 view;
	mat4 MVP;
}ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragLight;
layout(location = 4) out vec3 fragPos;

layout(push_constant) uniform PushConstants
{
	mat4 transformation;
	vec3 booleans;
	//vec3 position;
	//vec3 rotation;
	//vec3 scale;
	//vec3 multiTexture;
}transform;

out gl_PerVertex
{	vec4 gl_Position;
};

layout(location = 5) out fragTexCoords
{
	vec2 fragTCxy;
	vec2 fragTCzx;
	vec2 fragTCyz;
	float bx;
	float by;
	float bz;
	bool multiTexture;
}fragTexCoordsOut;

void main()
{

	mat4 modelView = ubo.view * ubo.model;
	mat3 normalMatrix = mat3(modelView);
	vec3 lightLocation = vec3(0, 0, 200);
	float nLength = length(inNormal);
	vec3 blendWeights = abs(inNormal);

	vec4 newPos = (transform.transformation * vec4(inPosition, 1));

	gl_Position = ubo.MVP * newPos;
	fragColor = inColor;
	fragNormal = normalMatrix * inNormal;
	fragLight = normalMatrix * (lightLocation);
	fragTexCoord = inTexCoord;
	vec4 pPos4 = ubo.view * (ubo.model * newPos);
	fragPos = (pPos4).xyz;

	if (transform.booleans.x > 0)
	{
		fragTexCoordsOut.fragTCxy = inPosition.xy + nLength;
		fragTexCoordsOut.fragTCzx = inPosition.zx + nLength;
		fragTexCoordsOut.fragTCyz = inPosition.yz + nLength;
		fragTexCoordsOut.bx = blendWeights.x / nLength;
		fragTexCoordsOut.by = blendWeights.y / nLength;
		fragTexCoordsOut.bz = blendWeights.z / nLength;
	}
	else
	{
		fragTexCoordsOut.fragTCxy = inTexCoord;
		fragTexCoordsOut.fragTCzx = inTexCoord;
		fragTexCoordsOut.fragTCyz = inTexCoord;
		fragTexCoordsOut.bx = 2;
		fragTexCoordsOut.by = 0;
		fragTexCoordsOut.bz = 0;
	}
}

