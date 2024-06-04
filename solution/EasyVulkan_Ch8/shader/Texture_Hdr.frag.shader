#version 460
#pragma shader_stage(fragment)
#include "EOTF.h"

layout(location = 0) in vec2 i_TexCoord;
layout(location = 0) out vec4 o_Color;
layout(binding = 0) uniform sampler2D u_Texture;
layout(push_constant) uniform pushConstants {
	float brightnessScale;
};

float Reinhard(float v) {
    return v  / (1 + v);
}
vec3 Reinhard(vec3 v) {
	return vec3(Reinhard(v.r), Reinhard(v.g), Reinhard(v.b));
}

void main() {
	o_Color = texture(u_Texture, i_TexCoord);
	o_Color.rgb = InverseEotf_PQ(Reinhard(o_Color.rgb) * brightnessScale);
}