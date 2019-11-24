#version 450

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inViewPos;

layout(push_constant) uniform PushConsts {
	mat4 projection;
	mat4 model;
	mat4 view;
	vec3 camPos;
	float padding;
	vec3 lightDir;
} push_constants;

layout ( set = 0 , binding = 0) uniform UBOParams {
	vec4 lights[4];
	float exposure;
	float gamma;
} uboParams;

layout ( set = 0 , binding = 1) uniform samplerCube samplerIrradiance;
layout ( set = 0 , binding = 2) uniform sampler2D samplerBRDFLUT;
layout ( set = 0 , binding = 3) uniform samplerCube prefilteredMap;
layout ( set = 0 , binding = 4) uniform sampler2D albedoMap;
layout ( set = 0 , binding = 5) uniform sampler2D normalMap;
layout ( set = 0 , binding = 6) uniform sampler2D aoMap;
layout ( set = 0 , binding = 7) uniform sampler2D metallicMap;
layout ( set = 0 , binding = 8) uniform sampler2D roughnessMap;

layout (location = 0) out vec4 outColor;

#define PI 3.1415926535897932384626433832795
#define ALBEDO pow(texture(albedoMap, inUV).rgb, vec3(2.2))
#define SHADOW_MAP_CASCADE_COUNT 4
#define ambients 0.3

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);

layout (set = 1, binding = 0) uniform sampler2DArray shadowMap;

layout (set = 1, binding = 1) uniform casUBO{
	float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];
	mat4 cascadeViewProjMat[SHADOW_MAP_CASCADE_COUNT];
} casUBOParams;

float textureProj(vec4 shadowCoord, vec2 offset, uint cascadeIndex)
{
	float shadow = 1.0;
	float bias = 0.005;

	if ( shadowCoord.z > 0.0 && shadowCoord.z < 1.0 ) {
		float dist = texture(shadowMap, vec3(shadowCoord.st + offset, cascadeIndex)).r;
		if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) {
			shadow = ambients;
		}
	}
	return shadow;

}

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

vec3 specularContribution(vec3 L, vec3 V, vec3 N, vec3 F0, float metallic, float roughness)
{
	vec3 H = normalize (V + L);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);

	vec3 lightColor = vec3(1.0);

	vec3 color = vec3(0.0);

	if (dotNL > 0.0) {
		float D = D_GGX(dotNH, roughness); 
		float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
		vec3 F = F_Schlick(dotNV, F0);		
		vec3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.001);		
		vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);			
		color += (kD * ALBEDO / PI + spec) * dotNL;
	}

	return color;
}
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
	vec3 N = perturbNormal();
	vec3 V = normalize(push_constants.camPos - inWorldPos);
	vec3 R = reflect(-V, N); 
	float metallic = texture(metallicMap, inUV).r;
	float roughness = texture(roughnessMap, inUV).r;
	
	vec3 F0 = vec3(0.04); 
	F0 = mix(F0, ALBEDO, metallic);
	vec3 Lo = vec3(0.0);
	
	for(int i = 0; i < uboParams.lights[i].length(); i++) {
		vec3 L = normalize(uboParams.lights[i].xyz - inWorldPos);
		Lo += specularContribution(L, V, N, F0, metallic, roughness);
	}   
	vec2 brdf = texture(samplerBRDFLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
	vec3 reflection = prefilteredReflection(R, roughness).rgb;	
	vec3 irradiance = texture(samplerIrradiance, N).rgb;
	vec3 diffuse = irradiance * ALBEDO;	
	vec3 F = F_SchlickR(max(dot(N, V), 0.0), F0, roughness);
	vec3 specular = reflection * (F * brdf.x + brdf.y);
	vec3 kD = 1.0 - F;
	kD *= 1.0 - metallic;	  
	vec3 ambient = (kD * diffuse + specular) * texture(aoMap, inUV).rrr;
	vec3 color = Lo + ambient;
	
	
	// shadow 
	uint cascadeIndex = 0;
	for(uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; ++i) {
		if(inViewPos.z < -casUBOParams.cascadeSplits[i]) {	
			cascadeIndex = i + 1;
		}
	}
	vec4 shadowCoord = (biasMat * casUBOParams.cascadeViewProjMat[cascadeIndex]) * vec4(inWorldPos, 1.0);	
	float shadow = textureProj(shadowCoord / shadowCoord.w, vec2(0.0), cascadeIndex);
	color *= shadow;
	
	// Tone mapping
	color = Uncharted2Tonemap(color * uboParams.exposure);
	color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));	
	// Gamma correction
	color = pow(color, vec3(1.0f / uboParams.gamma));
	outColor = vec4(color, 1.0);
}


