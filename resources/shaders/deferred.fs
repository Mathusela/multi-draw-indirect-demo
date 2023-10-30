#version 460
#define MAX_LIGHTS 1000

in vec2 fUV;

uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D colorTexture;

uniform vec3 cameraPos;

out vec4 fCol;

const float linearFalloff = 0.09;
const float quadraticFalloff = 0.032;
const float specularStrength = 0.3;
const float shininess = 64.0;

struct Light {
	vec3 position;
	vec3 color;
	float power;
};

layout (std140) uniform LightsBlock {
	Light lights[MAX_LIGHTS];
};
uniform int lightCount;

void main() {
	vec3 fPos = texture2D(positionTexture, fUV).xyz;
	vec3 fNormal = texture2D(normalTexture, fUV).xyz;
	vec3 color = texture2D(colorTexture, fUV).xyz;
	// Use depth buffer, would need to copy framebuffer depth buffer into default depth buffer
	if (fNormal == vec3(0.0)) discard;

	vec3 lighting = vec3(0.0);
	vec3 unitNormal = normalize(fNormal);

	for (int i=0; i<lightCount; i++) {
		Light light = lights[i];

		vec3 lightDir = normalize(light.position - fPos);
		vec3 cameraDir = normalize(cameraPos - fPos);
      	vec3 halfwayDir = normalize(lightDir + cameraDir);

		vec3 diffuse = max(dot(lightDir, unitNormal), 0.0) * color * light.color;

		vec3 specular = specularStrength * pow(max(dot(unitNormal, halfwayDir), 0.0), shininess) * light.color;

		float distance = length(lights[i].position - fPos);
      	float attenuation = 1.0 / (1.0 + linearFalloff * distance + quadraticFalloff * (distance * distance));
		
		lighting += (diffuse + specular)*attenuation*light.power;
		// lighting += (specular)*attenuation*light.power;
	}
	lighting += 0.05 * color;

	fCol = vec4(lighting, 1.0);
	// fCol = vec4(vec3(lights[0].color), 1.0);
}