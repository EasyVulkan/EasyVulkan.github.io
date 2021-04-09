#version 450
#pragma shader_stage(fragment)

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec3 i_Normal;
layout(location = 2) in vec4 i_AlbedoSpecular;
layout(location = 0) out vec4 o_Position;
layout(location = 1) out vec4 o_Normal;
layout(location = 2) out vec4 o_AlbedoSpecular;

void main() {
	o_Position = vec4(i_Position, 1);
	o_Normal = vec4(normalize(i_Normal), 1);
	o_AlbedoSpecular = i_AlbedoSpecular;
}