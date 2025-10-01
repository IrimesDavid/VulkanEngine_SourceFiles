#version 450

layout(set = 0, binding = 1) uniform samplerCube skybox;

layout(location = 0) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

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

void main() {

    outColor = texture(skybox, fragTexCoord);
}