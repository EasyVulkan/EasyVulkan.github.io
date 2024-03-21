#version 460
#pragma shader_stage(fragment)

layout(location = 0) out vec4 o_Color;

void main() {
	o_Color = vec4(0, 0.5, 1, 1);
}