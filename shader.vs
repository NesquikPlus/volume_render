#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aTexCoord;

uniform mat4 projection;
out vec3 TexCoord;

void main()
{
	gl_Position = projection * vec4(aPos, 1.0f);
	TexCoord = vec3(aTexCoord.x, aTexCoord.y, aTexCoord.z);
}