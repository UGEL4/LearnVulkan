#version 450

vec2 positions[3] = vec2[](vec2(0.0, -0.5), vec2(0.5, 0.5), vec2(-0.5, 0.5));

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 texCoord;

layout(binding = 0) uniform UniformBufferObj
{
	mat4 model;
	mat4 view;
	mat4 proj;
}ubo;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outTexCoord;
void main()
{
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(position, 1.0);
	outColor = color;
	outTexCoord = texCoord;
}