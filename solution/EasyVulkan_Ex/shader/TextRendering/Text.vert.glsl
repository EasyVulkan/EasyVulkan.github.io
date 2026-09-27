#version 460
#pragma shader_stage(vertex)

vec2 positions[4] = {
	{ 0, 0 },
	{ 0, 1 },
	{ 1, 0 },
	{ 1, 1 }
};

layout(location = 0) in vec2 i_Position;
layout(location = 1) in float i_Scale;
layout(location = 2) in vec4 i_Color; //Not premultiplied alpha
layout(location = 3) in uint i_LayerIndex;
layout(location = 4) in uint i_OffsetU;
layout(location = 5) in uint i_SizeU;
layout(location = 0) out vec3 o_TexCoord;
layout(location = 1) out vec4 o_Color;//Premultiplied alpha
layout(push_constant) uniform pushConstants {
	vec2 viewportSize;
	vec2 offset;
	vec2 textureSize;
};

void main() {
	gl_Position = vec4(
		(i_Position + i_Scale * vec2(i_SizeU, textureSize.y) * positions[gl_VertexIndex] + offset) * 2 / viewportSize - 1,
		0, 1);
	o_TexCoord = vec3(positions[gl_VertexIndex], i_LayerIndex);
	o_TexCoord.x = (i_OffsetU + o_TexCoord.x * i_SizeU) / textureSize.x;
	o_Color = vec4(i_Color.rgb * i_Color.a, i_Color.a);
}