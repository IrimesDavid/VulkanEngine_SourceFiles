#version 450

layout (location = 0) in vec2 fragOffset;
layout (location = 0) out vec4 outColor;

struct Light{
	vec4 position;
	vec4 color;
};

layout(set = 0, binding = 0) uniform GlobalUbo {
	mat4 projection;
    mat4 view;
	mat4 inverseView;
	//vec3 directionToLight;
	vec4 ambientLightColor;
	Light lights[10];
	int numLights;
} ubo;

layout (push_constant) uniform Push{
	vec4 position;
	vec4 color;
	float radius;
} push;

const float M_PI = 3.14159265358979323846;
void main(){
    float dist = sqrt(dot(fragOffset, fragOffset));
	if(dist >= 1.0) {
		discard;
	}
	float cosDis = 0.5f * (cos(dist*M_PI) + 1.0f);
	outColor = vec4(push.color.xyz + cosDis, cosDis);
}