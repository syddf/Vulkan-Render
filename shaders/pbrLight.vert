#version 450
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outViewPos;

layout(push_constant) uniform PushConsts {
	mat4 projection;
	mat4 model;
	mat4 view;
	vec3 camPos;
	float padding;
	vec3 lightDir;
} push_constants;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{
	mat4 inverseViewMat = inverse(push_constants.view);
	vec3 locPos = vec3(push_constants.model * vec4(inPos, 1.0));
	outWorldPos = locPos;
	outNormal = ( transpose( inverse( push_constants.model ) )   * vec4( inNormal , 0.0f ) ).xyz ;
	outUV = inUV;
	outUV.t = 1.0 - inUV.t;
	outViewPos = vec3(push_constants.view * vec4( outWorldPos , 1.0f )); 
	gl_Position =  push_constants.projection * push_constants.view * vec4(outWorldPos, 1.0);
}
