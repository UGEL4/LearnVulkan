#version 450

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D TexSampler;

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 intexCoord;
void main()
{
	//outColor = vec4(inColor, 1.0);
	outColor = texture(TexSampler, intexCoord);
}