#version 460
#pragma shader_stage(vertex)

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec3 i_Normal;
layout(location = 2) in vec4 i_AlbedoSpecular;
layout(location = 3) in vec3 i_InstancePosition;
layout(location = 0) out vec4 o_NormalZ;
layout(location = 1) out vec4 o_AlbedoSpecular;
layout(binding = 0) uniform descriptorConstants_pv{
	mat4 proj;
	mat4 view;
};

void main() {
	vec3 position = i_Position + i_InstancePosition;
	gl_Position = proj * view * vec4(position, 1);
	o_NormalZ.xyz = i_Normal;
	o_NormalZ.w = gl_Position.w;
	o_AlbedoSpecular = i_AlbedoSpecular;
}