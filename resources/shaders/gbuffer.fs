#version 460

layout(early_fragment_tests) in;

in vec3 fPos;
in vec3 fNormal;
in vec3 color;

uniform vec3 cameraPos;

layout (location = 0) out vec3 positionOut;
layout (location = 1) out vec3 normalOut;
layout (location = 2) out vec3 colorOut;

void main() {
	positionOut = fPos;
	normalOut = normalize(fNormal);
	colorOut = color;
}