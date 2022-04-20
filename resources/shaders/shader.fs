#version 450 core

out vec4 FragColor;

in vec3 TexCoord;

uniform sampler3D texture1;

void main()
{
	float amplitude = texture(texture1, TexCoord).x;
	FragColor = vec4(amplitude, amplitude, amplitude, amplitude);
}