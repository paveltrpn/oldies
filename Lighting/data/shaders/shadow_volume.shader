<vertex>

attribute vec4 s_attribute_0;

uniform vec4 light;

void main() {
	
	vec3 dir = s_attribute_0.xyz - light.xyz;
	float dist = length(dir);
	dir /= dist;
	
	if(dist > light.w) dir *= 0.0;
	else dir *= (light.w - dist);
	
	vec4 vertex = vec4(s_attribute_0.xyz + dir * (1.0 - s_attribute_0.w),1);
	
	gl_Position = gl_ModelViewProjectionMatrix * vertex;
	
	gl_FrontColor = vec4(0,0.1,0,1);
}

<fragment_depth_clamp>

!!ARBfp1.0

MOV_SAT result.depth, fragment.position.z;
MOV result.color, fragment.color;

END
