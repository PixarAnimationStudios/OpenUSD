#version 330
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 wirecolor;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec3 uvws;
layout (location = 4) in vec4 color;

out vec3 vertex_position;
out vec3 vertex_normal;
out vec3 vertex_tangent;
out vec3 vertex_uvws;
out vec4 vertex_color;

mat4 extractRotationMatrix(mat4 m){
return mat4(m[0].xyzw,
			m[1].xyzw,
			m[2].xyzw,
			0.0,0.0,0.0,1.0);
}

void main(){
	vertex_uvws = uvws;
	vertex_color = color;
	vertex_position = vec3(view * model * vec4(position,1.0));
	vertex_normal = vec3(model * vec4(normal,0.0));
	vertex_tangent = vec3(model * vec4(tangent,0.0));
	gl_Position = projection * vec4(vertex_position,1.0);
}
  