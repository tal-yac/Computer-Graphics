#version 330

in vec2 texCoord0;
in vec3 normal0;
in vec3 color0;
in vec3 position0;

uniform sampler2D sampler1;
uniform vec4 coeffs;
uniform vec4 root1;
uniform vec4 root2;
uniform vec4 root3;
uniform int iteration_num;
uniform float zoom;
uniform vec4 translate;

out vec4 Color;

vec2 complex_mul(vec2 z1, vec2 z2){
	return vec2((z1.x * z2.x) - (z1.y * z2.y), (z1.x * z2.y) + (z1.y * z2.x));
}

vec2 f(vec2 z){
	vec2 f_calc = coeffs[0] * complex_mul(z, complex_mul(z, z)) + coeffs[1] * complex_mul(z, z) + coeffs[2] * z;
	f_calc.x += coeffs[3];
	return f_calc;	
}

vec2 df(vec2 z){
	vec2 df_calc = 3.0 * coeffs[0] * complex_mul(z, z) + 2.0 * coeffs[1] * z;
	df_calc.x += coeffs[2];
	return df_calc;    
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
	vec2 z = (texCoord0 - 0.5 + translate.xy) * zoom;
    
	int i;
	for(i = 0; i < iteration_num; i++){
		vec2 complex_div_res = complex_div(f(z), df(z));
		z -= complex_div_res;
		if(length(complex_div_res) < 0.00001)
		  break;
	}


	return z;
}

void main()
{
	vec2 final_z = calc_z();
	float distance_1 = distance(final_z, root1.xy), distance_2 = distance(final_z, root2.xy), distance_3 = distance(final_z, root3.xy);
	Color = (distance_1 <= distance_2 && distance_1 <= distance_3) ? vec4(1, 0, 0, 1) :
	        (distance_2 <= distance_3) ? vec4(0, 1, 0, 1) :
			vec4(0, 0, 1, 1);
	// Color = vec4(0,1,0,1);
}
