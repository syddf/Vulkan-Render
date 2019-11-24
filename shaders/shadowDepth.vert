#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;

#define SHADOW_MAP_CASCADE_COUNT 4

layout(push_constant) uniform PushConsts {
	mat4 model;
	uint cascadeIndex;
} pushConsts;

layout (binding = 0) uniform UBO {
	float splitDepth[4];
	mat4[SHADOW_MAP_CASCADE_COUNT] cascadeViewProjMat;
} ubo;

layout (location = 0) out vec2 outUV;

out gl_PerVertex {
	vec4 gl_Position;   
};

void main()
{
	outUV = inUV;
	vec3 pos = inPos;
	gl_Position =  ubo.cascadeViewProjMat[pushConsts.cascadeIndex] * pushConsts.model *  vec4(pos, 1.0);
}