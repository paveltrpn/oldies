<vertex>

attribute vec3 s_attribute_0;
attribute vec3 s_attribute_1;
attribute vec3 s_attribute_2;
attribute vec3 s_attribute_3;
attribute vec4 s_attribute_4;

void main() {
	
	gl_Position = gl_ModelViewProjectionMatrix * vec4(s_attribute_4.x,s_attribute_4.y,0.0,1.0);
	
	gl_TexCoord[0] = s_attribute_4;
	gl_TexCoord[0].z = 0.0;
	
	// make 66.93 windows driver happy
	vec3 dir = s_attribute_0;
	gl_TexCoord[1].x = dot(dir,s_attribute_2);
	gl_TexCoord[1].y = dot(dir,s_attribute_3);
	gl_TexCoord[1].z = dot(dir,s_attribute_1);
}

<fragment>

!!ARBfp1.0

OPTION NV_fragment_program2;

TEMP color, temp, lockup, texcoord;
MOV texcoord, fragment.texcoord[0];

LOOP { 255, 0, 1 };
	
	TEX lockup, texcoord, texture[1], 3D;
	ADD texcoord.z, texcoord.z, 0.0039215686;
	
	TEX temp, lockup, texture[0], 2D;
	MAD color, temp, lockup.z, color;
	
	SUBC lockup.z, lockup.z, 0.000001;
	BRK (LT.z);
	
ENDLOOP;	

MOV result.color, color;

END
