#version 450

layout(binding = 4) uniform sampler2D TexSampler;

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inWorldPos;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outAlbedo;

void main()
{
	outPosition = vec4(inWorldPos, 1.0);
	outNormal = vec4(inColor, 1.0);
	outAlbedo = texture(TexSampler, inTexCoord);
}