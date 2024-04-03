#version 460
#pragma shader_stage(fragment)

struct light {
	vec3 position;
	vec3 color;
	float strength;
};
layout(constant_id = 0) const uint maxLightCount = 32;
layout(constant_id = 1) const uint shininess = 32;

layout(location = 0) out vec4 o_Color;
layout(binding = 0, input_attachment_index = 0) uniform subpassInput u_GBuffer[3];
layout(binding = 1) uniform descriptorConstants {
	vec3 cameraPosition;
	int lightCount;
	light lights[maxLightCount];
};

void main() {
	vec4 position = subpassLoad(u_GBuffer[0]);
	vec4 normal = subpassLoad(u_GBuffer[1]);
	vec4 albedoSpecular = subpassLoad(u_GBuffer[2]);
	o_Color = vec4(0, 0, 0, 1);
	for (uint i = 0; i < lightCount; i++) {
		vec3 toLight = lights[i].position - position.xyz;
		//No Ambient
		//Diffuse
		vec3 lightIntensity = lights[i].color * lights[i].strength / pow(length(toLight), 2);//Light strength follows inverse-square law in vacuum.
		toLight = normalize(toLight);
		vec3 diffuse = albedoSpecular.xyz * lightIntensity * max(dot(normal.xyz, toLight), 0);
		//Specular
		vec3 halfway = normalize(/*toCamera*/normalize(cameraPosition - position.xyz) + toLight);
		vec3 specular = albedoSpecular.w * lightIntensity * pow(
			max(dot(normal.xyz, halfway), 0),
			shininess);
		//Final Color
		o_Color.xyz += diffuse + specular;
	}
}