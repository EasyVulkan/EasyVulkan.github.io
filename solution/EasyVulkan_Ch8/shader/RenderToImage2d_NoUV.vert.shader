#version 460
#pragma shader_stage(vertex)

vec2 positions[4] = {
	{-1,-1},
	{-1, 1},
	{ 1,-1},
	{ 1, 1}
};

void main() {
	gl_Position = vec4(positions[gl_VertexIndex], 0, 1);
}