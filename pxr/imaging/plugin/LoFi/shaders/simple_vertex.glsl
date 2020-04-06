#version 330
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

in vec3 position;
void main(){
    vec3 p = vec3(view * model * vec4(position,1.0));
    gl_Position = projection * vec4(p,1.0);
}