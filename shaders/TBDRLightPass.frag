#version 450
#extension GL_ARB_separate_shader_objects : enable 

layout (location = 0) in vec2 inUV;

layout (set = 0 , binding = 0) uniform sampler2D albedoMap;
layout (set = 0 , binding = 1) uniform sampler2D positionMap;
layout (set = 0 , binding = 2) uniform sampler2D normalMap;
layout (set = 0 , binding = 3) uniform sampler2D PBRMap;
layout (set = 0 , binding = 4) uniform samplerCube samplerIrradiance;
layout (set = 0 , binding = 5) uniform sampler2D samplerBRDFLUT;
layout (set = 0 , binding = 6) uniform samplerCube prefilteredMap;

const int TILE_SIZE = 16;

struct PointLight
{
	vec3 pos;
	float radius;
	vec3 intensity;
	float padding;
};

#define MAX_POINT_LIGHT_PER_TILE 1023

struct LightVisible
{
	uint count;
	uint lightindices[MAX_POINT_LIGHT_PER_TILE];
};

layout(push_constant) uniform PushConstantObject
{
	ivec2 tileNum;
	ivec2 viewportOffset;
	mat4 projection;
	mat4 model;
	mat4 view;
	vec3 camPos;
	float exposure;
	float gamma;
} push_constants;

layout( std430 , set = 1,  binding = 0 ) buffer TileLightVisibility
{
	LightVisible light_visibilities[];
};

layout( set = 1 , binding = 1 ) uniform readonly PointLights
{
	int light_num;
	PointLight pointlights[20000];
};

layout (location = 0) out vec4 outColor;

#define PI 3.1415926535897932384626433832795
#define ALBEDO pow(texture(albedoMap, inUV).rgb, vec3(2.2))

vec3 Uncharted2Tonemap(vec3 x)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

float D_GGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom*denom); 
}

float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

vec3 F_Schlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 prefilteredReflection(vec3 R, float roughness)
{
	const float MAX_REFLECTION_LOD = 9.0;
	float lod = roughness * MAX_REFLECTION_LOD;
	float lodf = floor(lod);
	float lodc = ceil(lod);
	vec3 a = textureLod(prefilteredMap, R, lodf).rgb;
	vec3 b = textureLod(prefilteredMap, R, lodc).rgb;
	return mix(a, b, lod - lodf);
}

vec3 specularContribution(vec3 worldPos , vec3 V, vec3 N, vec3 F0, float metallic, float roughness , uint lightid)
{
	PointLight light = pointlights[lightid];
	vec3 L = normalize(light.pos - worldPos);
	if( distance(light.pos, worldPos) > light.radius )
	{	
		return vec3( 0.0f );
	}
	vec3 H = normalize (V + L);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);

	vec3 lightColor = light.intensity;

	vec3 color = vec3(0.0);

	if (dotNL > 0.0) {
		float D = D_GGX(dotNH, roughness); 
		float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
		vec3 F = F_Schlick(dotNV, F0);		
		vec3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.001);		
		vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);			
		color += (kD * ALBEDO / PI + spec) * dotNL * lightColor;
	}
	
	return color;
}

void main()
{
	vec3 N = texture(normalMap, inUV).xyz;
	vec3 inWorldPos = texture(positionMap , inUV).xyz;
	vec3 V = normalize(push_constants.camPos - inWorldPos);
	vec3 R = reflect(-V, N); 
	float roughness = texture(PBRMap, inUV).r;
	float metallic = texture(PBRMap, inUV).g;
	
	vec3 F0 = vec3(0.04); 
	F0 = mix(F0, ALBEDO, metallic);
	vec3 Lo = vec3(0.0);
	
	ivec2 tile_id = ivec2( ( gl_FragCoord.xy - push_constants.viewportOffset ) / TILE_SIZE ) ;
	uint tile_index = tile_id.y * push_constants.tileNum.x + tile_id.x;
	uint tile_light_num = light_visibilities[tile_index].count;
	for(int i = 0; i < tile_light_num; i++) {
		Lo += specularContribution( inWorldPos , V, N, F0, metallic, roughness , light_visibilities[tile_index].lightindices[i]);
	}   
	vec2 brdf = texture(samplerBRDFLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
	vec3 reflection = prefilteredReflection(R, roughness).rgb;	
	vec3 irradiance = texture(samplerIrradiance, N).rgb;
	vec3 diffuse = irradiance * ALBEDO;	
	vec3 F = F_SchlickR(max(dot(N, V), 0.0), F0, roughness);
	vec3 specular = reflection * (F * brdf.x + brdf.y);
	vec3 kD = 1.0 - F;
	kD *= 1.0 - metallic;	  
	vec3 ambient = (kD * diffuse + specular) * texture(PBRMap, inUV).bbb;
	vec3 color = Lo + ambient ;
	color = Uncharted2Tonemap(color * push_constants.exposure);
	color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));	
	color = pow(color, vec3(1.0f / push_constants.gamma));
	float alpha = 1.0f;
//	if( color.r == 0.0f && color.g == 0.0f && color.b == 0.0f ) alpha = 0.0f;
	outColor = vec4(color, alpha);
}