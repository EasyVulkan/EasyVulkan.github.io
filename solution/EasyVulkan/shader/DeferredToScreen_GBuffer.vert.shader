#version 450
#pragma shader_stage(vertex)

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec3 i_Normal;
layout(location = 2) in vec4 i_AlbedoSpecular;
layout(location = 0) out vec3 o_Position;
layout(location = 1) out vec3 o_Normal;
layout(location = 2) out vec4 o_AlbedoSpecular;
layout(binding = 0) uniform descriptorConstants_pv{
	mat4 proj;
	mat4 view;
};
layout(set = 1, binding = 0) uniform descriptorConstants_m{
	mat4 model;
};

void main() {
	o_Position = (model * vec4(i_Position, 1)).xyz;
	gl_Position = proj * view * vec4(o_Position, 1);
	o_Normal = transpose(inverse(mat3(model))) * normalize(i_Normal);
	o_AlbedoSpecular = i_AlbedoSpecular;
}