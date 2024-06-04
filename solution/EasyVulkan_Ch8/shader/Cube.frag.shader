#version 460
#pragma shader_stage(fragment)

layout(location = 0) in vec3 i_UVS;
layout(location = 0) out vec4 o_Color;
layout(set = 1, binding = 0) uniform samplerCube u_Texture;

void main() {
	o_Color = texture(u_Texture, i_UVS);
}