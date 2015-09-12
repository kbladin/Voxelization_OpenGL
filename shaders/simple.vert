#version 410 core

layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec3 vertexNormal_modelspace;

out vec3 normal_viewspace;
out vec3 vertexPosition_viewspace;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

void main(){
	vertexPosition_viewspace = vec3(V * M * vec4(vertexPosition_modelspace,1));
	normal_viewspace = vec3(V * M * vec4(vertexNormal_modelspace,0));
	gl_Position = P * V * M * vec4(vertexPosition_modelspace,1);
}


