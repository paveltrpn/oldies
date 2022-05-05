<vertex>

attribute vec3 s_attribute_0;
attribute vec3 s_attribute_1;
attribute vec3 s_attribute_2;
attribute vec3 s_attribute_3;
attribute vec4 s_attribute_4;

uniform vec3 light_pos;
uniform vec3 camera_pos;
uniform vec3 light_color;

void main() {
	
	gl_Position = gl_ModelViewProjectionMatrix * vec4(s_attribute_0,1.0);
	
	gl_TexCoord[0] = s_attribute_4;
	
	vec3 dir = light_pos - s_attribute_0;
	gl_TexCoord[1].x = dot(dir,s_attribute_2);
	gl_TexCoord[1].y = dot(dir,s_attribute_3);
	gl_TexCoord[1].z = dot(dir,s_attribute_1);
	
	dir = camera_pos - s_attribute_0;
	gl_TexCoord[2].x = dot(dir,s_attribute_2);
	gl_TexCoord[2].y = dot(dir,s_attribute_3);
	gl_TexCoord[2].z = dot(dir,s_attribute_1);
	
	gl_TexCoord[3].xyz = light_color;
}

<fragment>

uniform sampler2D s_texture_0;

void main() {
	
	vec3 light_dir = normalize(gl_TexCoord[1].xyz);
	vec3 camera_dir = normalize(gl_TexCoord[2].xyz);
	vec4 base = texture2D(s_texture_0,gl_TexCoord[0].xy);
	vec3 normal = vec3(0.0,0.0,1.0);
	
	gl_FragColor = base + pow(clamp(dot(reflect(-light_dir,normal),camera_dir),0.0,1.0),32.0) * 0.5 * gl_TexCoord[3];
}
