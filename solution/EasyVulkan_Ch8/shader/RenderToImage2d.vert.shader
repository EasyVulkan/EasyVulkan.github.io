#version 460
#pragma shader_stage(vertex)

vec2 positions[4] = {
	{-1,-1},
	{-1, 1},
	{ 1,-1},
	{ 1, 1}
};

layout(location = 0) out vec2 o_UV;

void main() {
	gl_Position = vec4(positions[gl_VertexIndex], 0, 1);
	o_UV = positions[gl_VertexIndex] / 2 + 0.5f;
}