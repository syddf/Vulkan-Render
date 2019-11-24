#version 450

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inWorldPos;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec4 outPBR;

layout (binding = 0) uniform sampler2D albedoMap;
layout (binding = 1) uniform sampler2D normalMap;
layout (binding = 2) uniform sampler2D aoMap;
layout (binding = 3) uniform sampler2D metallicMap;
layout (binding = 4) uniform sampler2D roughnessMap;

vec3 perturbNormal()
{
	vec3 tangentNormal = texture(normalMap, inUV).xyz * 2.0 - 1.0;
	vec3 q1 = dFdx(inWorldPos);
	vec3 q2 = dFdy(inWorldPos);
	vec2 st1 = dFdx(inUV);
	vec2 st2 = dFdy(inUV);
	vec3 N = normalize(inNormal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);
	return normalize(TBN * tangentNormal);
}

void main()
{
	outPosition = vec4( inWorldPos , 1.0f ) ;
	outNormal.y = -outNormal.y;
	outNormal = vec4( perturbNormal() , 1.0f );
	outAlbedo = texture(albedoMap, inUV);
	outPBR.r = texture(roughnessMap , inUV).r;
	outPBR.g = texture(metallicMap, inUV).r;
	outPBR.b = texture(aoMap , inUV).r;
	outPBR.a = 1.0f;
}