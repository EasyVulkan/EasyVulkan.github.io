#version 460
#pragma shader_stage(fragment)
#include "EOTF.h"

layout(location = 0) in vec2 i_TexCoord;
layout(location = 0) out vec4 o_Color;
layout(binding = 0) uniform sampler2D u_Texture;
layout(push_constant) uniform pushConstants {
	float brightnessScale;
};

void main() {
    o_Color = texture(u_Texture, i_TexCoord);
    if (o_Color.r < 1)
        o_Color.rgb = InverseEotf_PQ(o_Color.rgb * brightnessScale);
}