#version 450

#define PI 3.1415926535897932384626433832795

layout (location = 0) in vec3 inPos;
layout (location = 0) out vec4 outColor;
layout ( set = 0 , binding = 0) uniform samplerCube samplerEnv;

layout ( push_constant ) uniform PushConstants
{
	layout (offset = 64) float deltaPhi;
	layout (offset = 68) float deltaTheta;
}push_constant;


void main()
{
	vec3 N = normalize(inPos);
	vec3 up = vec3( 0.0f , 1.0f , 0.0f );
	vec3 right = normalize( cross( up , N ) );
	up = cross( N , right );
	vec3 color = vec3(0.0f);
	const float twoPi = 2 * PI;
	const float halfPi = 0.5f * PI;
	float count = 0;
	for( float phi = 0.0f ; phi < twoPi  ; phi += push_constant.deltaPhi )
	{
		for( float theta = 0.0f ; theta < halfPi ; theta += push_constant.deltaTheta ) 
		{
			vec3 tmp = cos(phi) * right + sin(phi) * up;
			vec3 dir = cos(theta) * N + sin( theta ) * tmp ;
			color += ( texture( samplerEnv , dir ).rgb * sin(theta) * cos(theta) ) ;
			count += 1.0f;
		}
	}
	outColor = vec4( PI * color / count  , 1.0f );
}