#version 460

layout(location=0) in vec3 vPos;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec3 positionIn;
layout(location=3) in vec3 colorIn;

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec3 fPos;
out vec3 fNormal;
out vec3 color;

void main() {
	color = colorIn;
	fNormal = vNormal;
	fPos = vPos + positionIn;
	gl_Position = projectionMatrix * viewMatrix * vec4(fPos, 1.0);
}