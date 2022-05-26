#version 330

// uniform vec4 lightColor;
uniform sampler2D sampler;

in vec2 texCoords0;
in vec3 normal0;
in vec3 color0;
in vec3 position0;

float color_range = 0.5;
vec3 threshold = vec3(0.00001, 0.00001, 0.00001);

vec3 palette[6] = {vec3(1.0, 0, 0),         vec3(0, 1.0, 0),
                   vec3(0, 0, 1.0),         vec3(1.0, 1.0, 0),
                   vec3(0.936, 0.5, 0.025), vec3(1.0, 1.0, 1.0)};

bool less_than_vec(vec3 v, vec3 u) {
  return v.x < u.x && v.y < u.y && v.z < u.z;
}

int vec_hash(vec3 v) {
  return less_than_vec(vec3(0.9, 0.5, 0.5) - v, threshold)  ? 0 :
		 less_than_vec(vec3(0.5, 0.9, 0.5) - v, threshold)  ? 1 :
		 less_than_vec(vec3(0.5, 0.5, 0.5) - v, threshold)  ? 2 :
		 less_than_vec(vec3(0.5, 0.05, 0.5) - v, threshold) ? 3 :
		 less_than_vec(vec3(0, 0.5, 0.5) - v, threshold)    ? 4 :
		 													  5 ;
}

out vec4 Color;
void main() {
  Color = texture2D(sampler, texCoords0) *
          vec4(palette[vec_hash(color0 * normal0 * color_range + color_range)],
               1.0); // you must have gl_FragColor
}
