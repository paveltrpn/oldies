<vertex>

attribute vec3 s_attribute_0;
attribute vec3 s_attribute_1;
attribute vec3 s_attribute_2;
attribute vec3 s_attribute_3;
attribute vec4 s_attribute_4;

void main() {
	
	gl_Position = gl_ModelViewProjectionMatrix * vec4(s_attribute_4.x,s_attribute_4.y,0.0,1.0);
	
	gl_TexCoord[0] = vec4(s_attribute_0,1.0);
}

<fragment>

void main() {
	
	gl_FragColor = gl_TexCoord[0];
}
