#version 450

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


layout(location = 0) out vec3 fragTexCoord;

// Cube vertices (no vertex buffer needed)
vec3 positions[36] = vec3[](
    // Front face
    vec3(-1.0, -1.0,  1.0), vec3( 1.0, -1.0,  1.0), vec3( 1.0,  1.0,  1.0),
    vec3( 1.0,  1.0,  1.0), vec3(-1.0,  1.0,  1.0), vec3(-1.0, -1.0,  1.0),
    // Back face
    vec3(-1.0, -1.0, -1.0), vec3(-1.0,  1.0, -1.0), vec3( 1.0,  1.0, -1.0),
    vec3( 1.0,  1.0, -1.0), vec3( 1.0, -1.0, -1.0), vec3(-1.0, -1.0, -1.0),
    // Left face
    vec3(-1.0,  1.0,  1.0), vec3(-1.0,  1.0, -1.0), vec3(-1.0, -1.0, -1.0),
    vec3(-1.0, -1.0, -1.0), vec3(-1.0, -1.0,  1.0), vec3(-1.0,  1.0,  1.0),
    // Right face
    vec3( 1.0,  1.0,  1.0), vec3( 1.0, -1.0, -1.0), vec3( 1.0,  1.0, -1.0),
    vec3( 1.0, -1.0, -1.0), vec3( 1.0,  1.0,  1.0), vec3( 1.0, -1.0,  1.0),
    // Bottom face
    vec3(-1.0, -1.0, -1.0), vec3( 1.0, -1.0, -1.0), vec3( 1.0, -1.0,  1.0),
    vec3( 1.0, -1.0,  1.0), vec3(-1.0, -1.0,  1.0), vec3(-1.0, -1.0, -1.0),
    // Top face
    vec3(-1.0,  1.0, -1.0), vec3(-1.0,  1.0,  1.0), vec3( 1.0,  1.0,  1.0),
    vec3( 1.0,  1.0,  1.0), vec3( 1.0,  1.0, -1.0), vec3(-1.0,  1.0, -1.0)
);

void main() {
    fragTexCoord = positions[gl_VertexIndex];
    
    // Remove translation from view matrix
    mat4 rotView = mat4(mat3(ubo.view));
    vec4 pos = ubo.projection * rotView * vec4(positions[gl_VertexIndex], 1.0);
    
    // Set z to w so that z/w = 1.0 (far plane)
    gl_Position = pos.xyww;
}