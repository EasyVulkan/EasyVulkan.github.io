#version 460
#pragma shader_stage(vertex)

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec3 i_Normal;
layout(location = 2) in vec4 i_AlbedoSpecular;
layout(location = 3) in vec3 i_InstancePosition;
layout(location = 0) out vec3 o_Position;
layout(location = 1) out vec3 o_Normal;
layout(location = 2) out vec4 o_AlbedoSpecular;
layout(push_constant) uniform pushConstants {
	mat4 proj;
};

void main() {
	o_Position = i_Position + i_InstancePosition;
	gl_Position = proj * vec4(o_Position, 1);
	o_Normal = i_Normal;
	o_AlbedoSpecular = i_AlbedoSpecular;
}