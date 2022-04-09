#version 330

in vec2 texCoord0;
in vec3 normal0;
in vec3 color0;
in vec3 position0;

uniform sampler2D sampler1;
uniform vec4 coeffs;
uniform vec2 root1;
uniform vec2 root2;
uniform vec2 root3;
uniform int iteration_num;

out vec4 Color;

vec2 f(vec2 z){
	float r = sqrt(pow(z.x, 2) + pow(z.y, 2));
	float angle = atan(z.y, z.x);
    float x = r * cos(angle), x_2 = pow(r, 2) * cos(2 * angle), x_3 = pow(r, 3) * cos(3 * angle);
	float y = r * sin(angle), y_2 = pow(r, 2) * sin(2 * angle), y_3 = pow(r, 3) * sin(3 * angle);
	float a = coeffs[0], b = coeffs[1], c = coeffs[2], d = coeffs[3];

	return vec2((a * x_3 + b * x_2 + c * x + d), (a * y_3 + b * y_2 + c * y));	
}

vec2 df(vec2 z){
	float r = sqrt(pow(z.x, 2) + pow(z.y, 2));
	float angle = atan(z.y, z.x);
    float x_2 = pow(r, 2) * cos(2 * angle), x_3 = pow(r, 3) * cos(3 * angle);
	float y_2 = pow(r, 2) * sin(2 * angle), y_3 = pow(r, 3) * sin(3 * angle);
	float a = coeffs[0], b = coeffs[1], c = coeffs[2];
    
	return vec2((3 * a * x_3 + 2 * b * x_2 + c), (3 * a * y_3 + 2 * b * y_2 + c));
}

vec2 complex_mul(vec2 z1, vec2 z2){
	return vec2((z1.x * z2.x) - (z1.y * z2.y), (z1.x * z2.y) + (z1.y * z2.x));
}

vec2 complex_conjugate(vec2 z){
	return vec2(z.x, -z.y);
}

vec2 complex_div(vec2 z1, vec2 z2){
	vec2 nominator = complex_mul(z1, complex_conjugate(z2));
	float denominator = complex_mul(z2, complex_conjugate(z2)).x;
	return nominator / denominator;
}

vec2 calc_z(){
	vec2 z = vec2((root1.x + root2.x + root3.x) / 3, (root1.y + root2.y + root3.z) / 3);
    
	int i;
	for(i = 0; i < iteration_num; ++i)
		z -= complex_div(f(z), df(z));

	return z;
}

float min3(float a, float b, float c) {
	return (a < b && a < c) ? a : (b < c) ? b : c;
}

void main()
{
	vec2 final_z = calc_z();
	float distance_1 = distance(final_z, root1), distance_2 = distance(final_z, root2), distance_3 = distance(final_z, root3);
	float min_distance = min3(distance_1, distance_2, distance_3);

	Color = (min_distance == distance_1) ? vec4(1, 0, 0, 1) :
	        (min_distance == distance_2) ? vec4(0, 1, 0, 1) :
			vec4(0, 0, 1, 1);
	//Color = vec4(1,0,0,1);
}
