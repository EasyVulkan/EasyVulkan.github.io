#version 450
#pragma shader_stage(fragment)

struct light {
	vec3 position;
	vec3 color;
	float strength;
};
layout(constant_id = 0) const int maxLightCount = 32;
layout(constant_id = 1) const int shininess = 32;

layout(location = 0) out vec4 o_Color;
layout(input_attachment_index = 0, binding = 0) uniform subpassInput u_Position;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput u_Normal;
layout(input_attachment_index = 2, binding = 2) uniform subpassInput u_AlbedoSpecular;
layout(binding = 3) uniform descriptorConstants{
	vec3 cameraPosition;
	int lightCount;
	light lights[maxLightCount];
};

void main() {
	vec4 position = subpassLoad(u_Position);
	vec4 normal = subpassLoad(u_Normal);
	vec4 albedoSpecular = subpassLoad(u_AlbedoSpecular);
	o_Color = vec4(0, 0, 0, 1);
	for (int i = 0; i < lightCount; i++) {
		vec3 lightToFrag = lights[i].position - position.xyz;
		//No Ambient
		//Diffuse
		vec3 lightColor = lights[i].color * lights[i].strength / pow(length(lightToFrag), 2);//Light strength follows inverse-square law in vacuum.
		lightToFrag = normalize(lightToFrag);
		vec3 diffuse = albedoSpecular.xyz * lightColor * max(dot(normal.xyz, lightToFrag), 0);
		//Specular
		vec3 halfwayDirection = normalize(normalize(/*cameraToFrag*/cameraPosition - position.xyz) + lightToFrag);
		vec3 specular = albedoSpecular.w * lightColor * pow(
			max(dot(normal.xyz, halfwayDirection), 0),
			shininess);
		//Final Color
		o_Color.xyz += diffuse + specular;
	}
}