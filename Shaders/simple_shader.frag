#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec2 fragUV;


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

layout(set = 1, binding = 0) uniform sampler2D diffTex;

layout(push_constant) uniform Push{
	mat4 modelMatrix;
	mat4 normalMatrix;
}push;

void main(){
	vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
	vec3 specularLight = vec3(0.0f);
	vec3 surfaceNormal = normalize(fragNormalWorld);

	vec3 cameraPosWorld = ubo.inverseView[3].xyz;
	vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld); // Direction from fragment to camera

	for(int i = 0; i < ubo.numLights; i++){
		Light light = ubo.lights[i];	
		
		// Diffuse light
		vec3 directionToLight = light.position.xyz - fragPosWorld;
		float attenuation = 1.0 / dot(directionToLight, directionToLight);
		directionToLight = normalize(directionToLight);


		float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0.0);
		vec3 intensity = light.color.xyz * light.color.w * attenuation;

		diffuseLight += intensity * cosAngIncidence;

		// Specular light
		vec3 halfAngle = normalize(viewDirection + directionToLight);
		float blinnTerm = dot(surfaceNormal, halfAngle);
		blinnTerm = clamp(blinnTerm, 0.0, 1.0);
		blinnTerm = pow(blinnTerm, 64.0); // Shininess factor (can be replaced with the specular map)
		specularLight += intensity * blinnTerm;
	}
	// Ambient light
	vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
	
	//Textures
	vec3 texDiff = texture(diffTex, fragUV).xyz;
	outColor = vec4(diffuseLight * texDiff + specularLight * texDiff, 1.0);
}