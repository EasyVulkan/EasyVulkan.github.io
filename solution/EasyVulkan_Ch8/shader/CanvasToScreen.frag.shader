#version 460
#pragma shader_stage(fragment)

layout(location = 0) in vec2 i_TexCoord;
layout(location = 0) out vec4 o_Color;
layout(binding = 0) uniform sampler2D u_Texture;

layout(push_constant) uniform pushConstants {
	layout(offset = 8)
	vec2 canvasSize;
};

void main() {
	o_Color = texture(u_Texture, i_TexCoord);
	o_Color.g = texture(u_Texture, i_TexCoord + 8 / canvasSize).r;
	o_Color.b = texture(u_Texture, i_TexCoord - 8 / canvasSize).r;
}