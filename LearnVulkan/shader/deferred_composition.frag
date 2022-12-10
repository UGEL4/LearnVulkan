#version 450

layout(binding = 1) uniform sampler2D samplerPosition;
layout(binding = 2) uniform sampler2D samplerNormal;
layout(binding = 3) uniform sampler2D samplerAlbedo;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

void main()
{
	// Get G-Buffer values
	vec3 fragPos = texture(samplerPosition, inUV).rgb;
	vec3 normal = texture(samplerNormal, inUV).rgb;
	vec4 albedo = texture(samplerAlbedo, inUV);

	// Ambient part
	vec3 fragcolor = albedo.rgb + normal;
	outColor = vec4(normal, 1.0);
}