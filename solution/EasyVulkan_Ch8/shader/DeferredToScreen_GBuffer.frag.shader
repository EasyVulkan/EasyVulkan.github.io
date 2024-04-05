#version 460
#pragma shader_stage(fragment)

layout(location = 0) in vec4 i_NormalZ;
layout(location = 1) in vec4 i_AlbedoSpecular;
layout(location = 0) out vec4 o_NormalZ;
layout(location = 1) out vec4 o_AlbedoSpecular;

void main() {
	o_NormalZ = i_NormalZ;
	o_AlbedoSpecular = i_AlbedoSpecular;
}