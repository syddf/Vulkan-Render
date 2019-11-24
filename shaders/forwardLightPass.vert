#version 450
#extension GL_ARB_separate_shader_objects : enable 

layout(push_constant) uniform PushConstantObject
{
	ivec2 tileNum;
	ivec2 viewportOffset;
	mat4 model;
	mat4 mvp;
} push_constants;


layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in vec3 in_normal;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_tex_coord;
layout(location = 2) out vec3 frag_normal;
layout(location = 3) out vec3 frag_pos_world;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	mat4 invtransmodel = transpose(inverse(push_constants.model));
	
	gl_Position = push_constants.mvp * vec4( in_position , 1.0f ) ;
	frag_color = in_color;
	frag_tex_coord = in_tex_coord;
	frag_normal = in_normal;
	frag_pos_world = vec3( push_constants.model * vec4(in_position , 1.0f));
}