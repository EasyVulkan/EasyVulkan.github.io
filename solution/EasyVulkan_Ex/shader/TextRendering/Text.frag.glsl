#version 460
#pragma shader_stage(fragment)

layout(location = 0) in vec3 i_TexCoord;
layout(location = 1) in vec4 i_Color;//Premultiplied alpha
layout(location = 0) out vec4 o_Color;
layout(binding = 0) uniform sampler2DArray u_Texture;

void main() {
	o_Color = i_Color * texture(u_Texture, i_TexCoord).r;
}