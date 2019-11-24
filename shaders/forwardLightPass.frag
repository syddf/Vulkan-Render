#version 450
#extension GL_ARB_separate_shader_objects : enable 

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
} push_constants;

layout( location = 0 ) in vec3 frag_color;
layout( location = 1 ) in vec2 frag_tex_coord;
layout( location = 2 ) in vec3 frag_normal;
layout( location = 3 ) in vec3 frag_pos_world;

layout( location = 0 ) out vec4 out_color;

layout( set = 0 , binding = 0 ) uniform MatUbo
{
	vec3 cameraPos;
}transform;

layout( set = 1 , binding = 0 ) uniform sampler2D albedo_sampler;
layout( set = 1 , binding = 1 ) uniform sampler2D normal_sampler;

layout( std430 ,  set = 2 , binding = 0 ) buffer TileLightVisibility
{
	LightVisible light_visibilities[];
};

layout( set = 2 , binding = 1 ) uniform readonly PointLights
{
	int light_num;
	PointLight pointlights[20000];
};

vec3 normalMap(vec3 geomnor, vec3 normap)
{
	if( normap.xyz == vec3(1.0f , 1.0f , 1.0f)) return geomnor;
    normap = normap * 2.0 - 1.0;
    vec3 up = normalize(vec3(0.001, 1, 0.001));
    vec3 surftan = normalize(cross(geomnor, up));
    vec3 surfbinor = cross(geomnor, surftan);
    return normalize(normap.y * surftan + normap.x * surfbinor + normap.z * geomnor);
}

void main()
{
	vec3 diffuse = texture( albedo_sampler , frag_tex_coord ).xyz;
	vec3 normal = normalMap( frag_normal , texture(normal_sampler, frag_tex_coord).rgb ) ;
	ivec2 tile_id = ivec2( ( gl_FragCoord.xy - push_constants.viewportOffset ) / TILE_SIZE ) ;
	uint tile_index = tile_id.y * push_constants.tileNum.x + tile_id.x;
	vec3 illuminance = vec3(0.0f);
	uint tile_light_num = light_visibilities[tile_index].count;
	out_color = vec4(0.0f);
	for( int i = 0 ; i < tile_light_num ; i ++ ) 
	{

		PointLight light = pointlights[light_visibilities[tile_index].lightindices[i]];
		vec3 lightDir = normalize(light.pos - frag_pos_world);
		float lambertian = max( dot( lightDir, normal) , 0.0f ) ;
		if( lambertian > 0.0f ) 
		{
			float light_distance = distance( light.pos , frag_pos_world ) ;
			if( light_distance > light.radius ) 
			{
				continue;
			}
			
			vec3 viewDir = normalize(transform.cameraPos - frag_pos_world);
			vec3 halfDir = normalize( viewDir + lightDir);
			float specAngle = max( dot( halfDir , normal ) , 0.0f );
			float specular = pow(specAngle , 32.0f);
			float att = clamp( 1.0f - ( light_distance * light_distance ) / ( light.radius * light.radius )  , 0.0f , 1.0f ) ;
			
			illuminance += light.intensity * att * ( lambertian * diffuse + specular ) ;
		}
	}
	
	out_color = vec4(illuminance , 1.0f);
}