#version 330

in vec3 vertex_position;
in vec3 vertex_normal;
in vec3 vertex_tangent;
in vec3 vertex_uvws;
in vec4 vertex_color;
    

uniform sampler2D tex;

//We need view matrix
uniform mat4 view;
uniform vec3 lightPosition;
uniform int selected;
uniform int wireframe;
uniform vec3 wirecolor;
uniform vec3 ambient;

out vec4 outColor;

void main()
{

	if(wireframe == 1){
		if(selected == 1)outColor = vec4(1.0,1.0,1.0,1.0);
		else outColor = vec4(wirecolor,1.0);
	}
	else{
	/*
// check if this is a directional light
if(lightPosition.w == 0.0) {
  // it is a directional light.
  // get the direction by converting to vec3 (ignore W) and negate it
  vec3 lightDirection = -lightPosition.xyz
} else {
  // NOT a directional light
}
		
		vec3 lightDirection = -normalize(lightPosition);
		float diffuse = max(0.0, dot(vertex_normal, normalize(lightPosition)));
		outColor = vec4(diffuse, diffuse, diffuse, 1.0);
		
		//Raise Light Position to eye space
		vec3 light_position_eye = vec3(view*vec4(lightPosition,1.0));
		vec3 distance_to_light_eye = light_position_eye - vertex_position;

		vec3 direction_to_light_eye = normalize(distance_to_light_eye);
		vec3 direction_to_camera = normalize( view[3].xyz);
		float d;
		if(dot(direction_to_camera,normalize(vertex_normal))<0){
			d = 1.0*dot(direction_to_light_eye,normalize(vertex_normal))+0.5;
			d = max(d,0.2);
		}
		else
			d = 0.2;
	
		float a = 1.0;
		
		//vec4 t = texture(tex,vertex_uvws.xz);//*d;
		//if((t.x+t.y+t.z)/3>0.5)a=0.0;
		vec3 color = vertex_color.xyz*d;
		outColor = vec4(color,a);
		*/
		
		vec3 lightDirection = -normalize(lightPosition);
		float d = dot(vertex_normal, normalize(lightPosition));
		if(gl_FrontFacing)
		{
			if(d<0) outColor = vec4(ambient,1.0) * vertex_color;
			else
			{
				vec3 lightColor = vec3(1.0,0.8,0.75);
				outColor = vec4(d*lightColor, 1.0) * vertex_color;
			}
		}
		else outColor = vec4(0.5,0.5,0.5,1.0) * vertex_color;
		
		//outColor = vec4((vertex_normal+vec3(1.0))*0.5, 1.0);

	}

}