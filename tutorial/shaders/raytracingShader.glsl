#version 330

uniform vec4 eye;
uniform vec4 ambient;
uniform vec4[20] objects;
uniform vec4[20] object_colors;
uniform vec4[10] lights_direction;
uniform vec4[10] lights_intensity;
uniform vec4[10] lights_position;
uniform ivec4 sizes;

in vec3 position0;
in vec3 normal0;

int objects_size() { return sizes.x; }
int lights_size() { return sizes.y; }

bool is_sphere(vec4 o) { return (o.w > 0); }
bool is_plane(vec4 o) { return (o.w <= 0); }

bool is_directional(vec4 o) { return (o.w < 0.5); }

float _pow(float b, float e) {
  return (b < 0 && int(e) % 2 == 1) ? -pow(-b, e) : pow(abs(b), e);
}

void swap(inout float x, inout float y) {
  float tmp = x;
  x = y;
  y = tmp;
}

float intersection(inout int source_index, vec3 p0, vec3 v) {
  float tmin = 999999999;
  int index = -1;
  for (int i = 0; i < objects_size(); ++i) {
    if (i == source_index)
      continue;
    float t;
    if (is_sphere(objects[i])) {
      vec3 l = objects[i].xyz - p0;
      float tm = dot(l, v);
      float d2 = dot(l, l) - tm * tm;
      float r2 = objects[i].w * objects[i].w;
      if (d2 > r2)
        continue;
      float th = sqrt(r2 - d2);
      t = (tm < 0) ? tm + th : tm - th;
    } else {
      vec3 n = objects[i].xyz;
      vec3 q0 = -objects[i].w * n;
      t = dot(n, q0 - p0) / dot(n, v);
    }
    if (0 < t && t < tmin) {
      tmin = t;
      index = i;
    }
  }
  source_index = index;
  return tmin;
}

vec3 snell_refract(vec3 l, vec3 n, inout float refractive_index_i, inout float refractive_index_r){
    float ln_dot_prod = clamp(dot(l, n), -1, 1);
    float ref_ir = refractive_index_i / refractive_index_r;
    float cos_theta_i = ln_dot_prod / (length(l) * length(n));
    float theta_i = acos(cos_theta_i);
    float theta_r = asin((refractive_index_i * sin(theta_i)) / refractive_index_r);
    vec3 t = ((ref_ir * cos_theta_i - cos(theta_r)) * n) - ref_ir * l;
    return t;
}

// body index in objects, point on surface of object, diffuse_factor for plane
// squares
vec3 color_calc(int source_index, vec3 p0, vec3 u, float diffuse_factor) {
  vec3 color = ambient.rgb * object_colors[source_index].rgb;
  float specular_coeff = 0.7f;
  for (int i = 0; i < lights_size(); i++)
  {
    vec3 v;
    if (is_directional(lights_direction[i])) {
      int index = source_index;
      v = normalize(lights_direction[i].xyz);
      //  v = normalize(vec3(0.0,0.5,-1.0));
      float t = intersection(index, p0, -v);
      if (index < 0 || is_plane(objects[index])) // no intersection
      {
        // vec3 u = normalize(p0 - eye.xyz);
        if (is_sphere(objects[source_index])) {
          vec3 n = -normalize(p0 - objects[source_index].xyz);
          vec3 refl = normalize(reflect(v, n));
          if (dot(v, n) > 0.0)
            color += max(specular_coeff * lights_intensity[i].rgb *
                             _pow(dot(refl, u), object_colors[source_index].a),
                         vec3(0.0, 0.0, 0.0)); // specular
          color += max(diffuse_factor * lights_intensity[i].rgb *
                           object_colors[source_index].rgb * dot(v, n),
                       vec3(0.0, 0.0, 0.0)); // difuse
          //        color = vec3(1.0,1.0,0.0);
        } else {
          vec3 n = objects[source_index].xyz;
          vec3 refl = normalize(reflect(v, n));
          color = min(
              color + max(specular_coeff * lights_intensity[i].rgb *
                              _pow(dot(refl, u), object_colors[source_index].a),
                          vec3(0.0, 0.0, 0.0)),
              vec3(1.0, 1.0, 1.0)); // specular
          color =
              min(color + max(diffuse_factor * lights_intensity[i].rgb *
                                  object_colors[source_index].rgb * dot(v, n),
                              vec3(0.0, 0.0, 0.0)),
                  vec3(1.0, 1.0, 1.0)); // difuse

          //      color = vec3(1.0,1.0,0.0);
        }
      }
      //   else if(index == 1)
      //          color = lights_intensity[i].rgb;

    } else // flashlight
    {
      int index = -1;
      v = -normalize(lights_position[i].xyz - p0);
      if (dot(v, normalize(lights_direction[i].xyz)) < lights_position[i].w) {
        continue;
      } else {
        // vec3 u = normalize(p0 - eye.xyz);
        float t = intersection(index, lights_position[i].xyz, v);
        if (index == source_index) // no intersection
        {
          if (is_sphere(objects[source_index])) {
            vec3 n = -normalize(p0 - objects[source_index].xyz);
            vec3 refl = normalize(reflect(v, n));
            if (dot(v, n) > 0.0)
              color += max(specular_coeff * lights_intensity[i].rgb *
                               _pow(dot(refl, u), object_colors[source_index].a),
                           vec3(0.0, 0.0, 0.0)); // specular
            color += max(diffuse_factor * lights_intensity[i].rgb *
                             object_colors[source_index].rgb * dot(v, n),
                         vec3(0.0, 0.0, 0.0));
            //          color = vec3(1.0,1.0,0.0);
          } else // plane
          {
            vec3 n = normalize(objects[source_index].xyz);
            vec3 refl = normalize(reflect(v, n)); // specular
            color = min(color + max(specular_coeff * lights_intensity[i].rgb *
                                        _pow(dot(refl, u),
                                            object_colors[source_index].a),
                                    vec3(0.0, 0.0, 0.0)),
                        vec3(1.0, 1.0, 1.0));
            color =
                min(color + max(diffuse_factor * lights_intensity[i].rgb *
                                    object_colors[source_index].rgb * dot(v, n),
                                vec3(0.0, 0.0, 0.0)),
                    vec3(1.0, 1.0, 1.0));
            // color = vec3(1.0,1.0,0.0);
          }
        }
        // else if(index == 1)
        //     color = lights_intensity[i].rgb;
      }
    }
  }
  //   color = vec3(1.0,1.0,0.0);
  return min(color, vec3(1.0, 1.0, 1.0));
}

void main() {
  vec3 v = normalize(position0 - eye.xyz);
  int index = -1;
  float t = intersection(index, position0, v);
  if (index < 0)
    return;
  //    gl_FragColor = vec4(1.0,1.0,1.0,1.0);

  // v= normalize( position0 - eye.xyz);
  // mirror
  int steps;
  vec3 n, p = position0 + t * v;
  float refractive_index_i = 1.0, refractive_index_r = 1.5;
  bool negate = false;

  for (steps = 5; steps > 0; --steps) {
    if(index < sizes.z || index < sizes.w){
      n = (is_sphere(objects[index])) ? normalize(p - objects[index].xyz)
                                      : normalize(objects[index].xyz);
      // if (index < sizes.w && negate) {
      //   n *= -1;
      // }
      v = (index < sizes.w)
          ? normalize(refract(v, n, refractive_index_i / refractive_index_r))
          : normalize(reflect(v, n));
      t = intersection(index, p, v);
      p += t * v;
      swap(refractive_index_i, refractive_index_r);
      // negate = !negate;

    }
  }
  float x = p.x; // max(abs(p.x),abs(p.y))*sign(p.x+p.y);
  float y =
      p.y; // max(min(abs(p.y),abs(p.x)),abs(p.z))*sign(min(abs(p.y),abs(p.x))+p.z);

  // if(objects[index].w <= 0 && (mod(int(abs(1.5*x)),2) ==
  // mod(int(abs(1.5*y)),2)))
  gl_FragColor = (is_plane(objects[index]) &&
                  (((mod(int(1.5 * x), 2) == mod(int(1.5 * y), 2)) &&
                    ((x > 0 && y > 0) || (x < 0 && y < 0))) ||
                   ((mod(int(1.5 * x), 2) != mod(int(1.5 * y), 2) &&
                     ((x < 0 && y > 0) || (x > 0 && y < 0))))))
                     ? vec4(color_calc(index, p, v, 0.5), 1)
                     : vec4(color_calc(index, p, v, 1.0), 1);
}
