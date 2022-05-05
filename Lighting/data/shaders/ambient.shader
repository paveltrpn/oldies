<vertex>

attribute vec3 s_attribute_0;
attribute vec3 s_attribute_1;

uniform vec3 camera;

void main() {
	
	gl_Position = gl_ModelViewProjectionMatrix * vec4(s_attribute_0,1);
	
	gl_FrontColor = vec4(dot(s_attribute_1,normalize(camera - s_attribute_0)) * 0.01);
}
