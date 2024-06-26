#version 460
#pragma shader_stage(vertex)

//Looking from outside, all counterclockwise triangles (if +z is deeper)
vec3 positions[14] = {
	{ 1, 1, 1},
	{-1,-1, 1},
	{-1, 1, 1},
	{-1, 1,-1},
	{ 1, 1, 1},

	{ 1, 1,-1},
	{ 1,-1,-1},
	{-1, 1,-1},

	{-1,-1,-1},
	{-1,-1, 1},
	{ 1,-1,-1},

	{ 1,-1, 1},
	{ 1, 1, 1},
	{-1,-1, 1}
};

layout(location = 0) out vec3 o_UVS;
layout(binding = 0) uniform descriptorConstants{
	mat4 proj;
	mat4 view;
	mat4 model;
};

void main() {
	//If the proj and view matrix are used for right handed coordinate:
	//gl_Position = proj * view * model * vec4(positions[gl_VertexIndex].xy, -positions[gl_VertexIndex].z, 1);
	gl_Position = proj * view * model * vec4(positions[gl_VertexIndex], 1);
	o_UVS = positions[gl_VertexIndex];
}