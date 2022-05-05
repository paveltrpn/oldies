<vertex>

attribute vec3 s_attribute_0;
attribute vec3 s_attribute_1;
attribute vec3 s_attribute_2;
attribute vec3 s_attribute_3;
attribute vec4 s_attribute_4;

uniform vec3 light_pos;
uniform vec3 light_color;

void main() {
	
	gl_Position = gl_ModelViewProjectionMatrix * vec4(s_attribute_4.x,s_attribute_4.y,0.0,1.0);
	
	vec3 dir = light_pos - s_attribute_0;
	gl_TexCoord[1].x = dot(dir,s_attribute_2);
	gl_TexCoord[1].y = dot(dir,s_attribute_3);
	gl_TexCoord[1].z = dot(dir,s_attribute_1);
	
	gl_TexCoord[2].xyz = light_color;
}

<fragment>

void main() {
	
	vec3 light_dir = normalize(gl_TexCoord[1].xyz);
	vec3 normal = vec3(0.0,0.0,1.0);
	
	gl_FragColor = dot(light_dir,normal) * gl_TexCoord[2];
}
