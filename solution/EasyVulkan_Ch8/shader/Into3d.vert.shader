#version 460
#pragma shader_stage(vertex)

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec4 i_Color;
layout(location = 2) in vec3 i_InstancePosition;
layout(location = 0) out vec4 o_Color;
layout(push_constant) uniform pushConstants {
	mat4 proj;
};

void main() {
	gl_Position = proj * vec4(i_Position + i_InstancePosition, 1);
	o_Color = i_Color;
}