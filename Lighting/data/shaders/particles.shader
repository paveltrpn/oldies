<vertex>

attribute vec4 s_attribute_0;
attribute vec4 s_attribute_1;
attribute vec3 s_attribute_2;

uniform mat3 tmodelview;
uniform vec3 color;

void main() {
	vec3 dx = tmodelview * vec3(s_attribute_1.x,0,0);
	vec3 dy = tmodelview * vec3(0,s_attribute_1.y,0);
	vec4 vertex = vec4(s_attribute_0.xyz + (dx + dy) * 2.0 * s_attribute_0.w,1);
	
	gl_Position = gl_ModelViewProjectionMatrix * vertex;
	
	gl_TexCoord[0].x = s_attribute_1.x * s_attribute_1.z - s_attribute_1.y * s_attribute_1.w + 0.5;
	gl_TexCoord[0].y = s_attribute_1.x * s_attribute_1.w + s_attribute_1.y * s_attribute_1.z + 0.5;
	gl_TexCoord[0].z = 0.0;
	gl_TexCoord[0].w = 1.0;
	
	gl_FrontColor = vec4(s_attribute_2 * color,1);
}
