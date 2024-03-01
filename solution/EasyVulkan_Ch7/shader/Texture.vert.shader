#version 460
#pragma shader_stage(vertex)

layout(location = 0) in vec2 i_Position;
layout(location = 1) in vec2 i_TexCoord;
layout(location = 0) out vec2 o_TexCoord;

void main() {
	gl_Position = vec4(i_Position, 0, 1);
	o_TexCoord = i_TexCoord;
}