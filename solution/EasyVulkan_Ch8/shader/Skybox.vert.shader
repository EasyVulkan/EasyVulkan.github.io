#version 460
#pragma shader_stage(vertex)

//Looking from inside, all counterclockwise triangles (if +z is deeper)
vec3 positions[14] = {
	{-1, 1, 1},
	{ 1,-1, 1},
	{ 1, 1, 1},
	{ 1, 1,-1},
	{-1, 1, 1},

	{-1, 1,-1},
	{-1,-1,-1},
	{ 1, 1,-1},

	{ 1,-1,-1},
	{ 1,-1, 1},
	{-1,-1,-1},

	{-1,-1, 1},
	{-1, 1, 1},
	{ 1,-1, 1}
};

layout(location = 0) out vec3 o_UVS;
layout(binding = 0) uniform descriptorConstants{
	mat4 proj;
	mat4 view;
};

void main() {
	//If the proj and view matrix are used for right handed coordinate:
	//gl_Position = proj * mat4(mat3(view)) * vec4(positions[gl_VertexIndex].xy, -positions[gl_VertexIndex].z, 1);
	gl_Position = proj * mat4(mat3(view)) * vec4(positions[gl_VertexIndex], 1);
	o_UVS = positions[gl_VertexIndex];
}