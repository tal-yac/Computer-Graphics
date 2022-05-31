#version 330

uniform vec4 lightColor;

in vec3 normal0;
out vec4 Color;
void main()
{
	Color = vec4(lightColor.r, abs(normal0));
}