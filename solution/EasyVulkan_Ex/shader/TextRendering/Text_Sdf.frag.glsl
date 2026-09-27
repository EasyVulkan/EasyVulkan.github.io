#version 460
#pragma shader_stage(fragment)

layout(location = 0) in vec3 i_TexCoord;
layout(location = 1) in vec4 i_Color;//Premultiplied alpha
layout(location = 0) out vec4 o_Color;
layout(binding = 0) uniform sampler2DArray u_Texture;
layout(push_constant) uniform pushConstants {
	layout(offset = 24)
	float pixelDistanceScale;
};

void main() {
	o_Color = i_Color * clamp((texture(u_Texture, i_TexCoord).r * 255 - 128) / pixelDistanceScale + 0.5f, 0, 1);
}