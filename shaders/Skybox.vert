#version 450
#extension GL_ARB_separate_shader_objects : enable 
layout(push_constant) uniform PushConstantObject
{
	mat4 mvp;
} push_constants;

layout (location = 0) in vec3 inPos;
layout (location = 0) out vec3 outUVW;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	outUVW = inPos;
	gl_Position = push_constants.mvp * vec4( inPos , 1.0f );
}