<vertex>

attribute vec3 s_attribute_0;
attribute vec3 s_attribute_1;
attribute vec3 s_attribute_2;
attribute vec3 s_attribute_3;
attribute vec2 s_attribute_4;
attribute vec4 s_attribute_5;

uniform vec4 light;
uniform float clip_radius;
uniform vec3 camera;
uniform vec3 color;

void main() {
	
	float dist = length(s_attribute_5.xyz - light.xyz);
	
	if(dist > clip_radius + s_attribute_5.w) {
		gl_Position = gl_ModelViewProjectionMatrix * vec4(100000.0,100000.0,100000.0,1.0);
	} else {
		gl_Position = gl_ModelViewProjectionMatrix * vec4(s_attribute_0,1.0);
	}
	
	gl_TexCoord[0] = vec4(s_attribute_4.x,s_attribute_4.y,0,1);
	
	vec3 dir = (light.xyz - s_attribute_0) / light.w;
	
	gl_TexCoord[1].x = dot(dir,s_attribute_2);
	gl_TexCoord[1].y = dot(dir,s_attribute_3);
	gl_TexCoord[1].z = dot(dir,s_attribute_1);
	gl_TexCoord[1].w = 1.0;
	
	dir = camera - s_attribute_0;
	
	gl_TexCoord[2].x = dot(dir,s_attribute_2);
	gl_TexCoord[2].y = dot(dir,s_attribute_3);
	gl_TexCoord[2].z = dot(dir,s_attribute_1);
	gl_TexCoord[2].w = 1.0;
	
	gl_FrontColor = vec4(color,1);
}

<fragment>

uniform sampler2D s_texture_0;
uniform sampler2D s_texture_1;

void main() {
	
	float attenuation = clamp(1.0 - dot(gl_TexCoord[1].xyz,gl_TexCoord[1].xyz),0.0,1.0);
	
	vec3 normal = normalize(texture2D(s_texture_1,gl_TexCoord[0].xy).xyz * 2.0 - 1.0);
	vec3 light_dir = normalize(gl_TexCoord[1].xyz);
	vec3 camera_dir = normalize(gl_TexCoord[2].xyz);
	
	vec4 base = texture2D(s_texture_0,gl_TexCoord[0].xy);
	
	gl_FragColor = (dot(light_dir,normal) * base +
		pow(clamp(dot(reflect(-light_dir,normal),camera_dir),0.0,1.0),16.0) * base.w) * gl_Color * attenuation;
}

<fragment_nv>

!!FP1.0

DP3H H0.w, f[TEX1], f[TEX1];	// attenuation
SUBH_SAT H0.w, 1.0, H0.w;

TEX H1, f[TEX0], TEX1, 2D;		// normal
MADX H1, H1, 2.0, -1.0;

TEX H2, f[TEX1], TEX2, CUBE;	// light dir
MADX H2, H2, 2.0, -1.0;

TEX H3, f[TEX2], TEX2, CUBE;	// camera dir
MADX H3, H3, 2.0, -1.0;

/*DP3X H4.x, H1, H1;
DP3X H4.y, H2, H2;
DP3X H4.z, H3, H3;
MADX H4, H4, -0.5, 0.5;
MADX H1, H1, H4.x, H1;
MADX H2, H2, H4.y, H2;
MADX H3, H3, H4.z, H3;*/

DP3X H1.w, H1, H2;				// diffuse

TEX H4, f[TEX0], TEX0, 2D;		// base texture
MULX H5, H4, H1.w;				// diffuse * base

RFLH H2, H1, H2;				// specular
DP3X_SAT H3.w, H3, H2;
POWH_SAT H3.w, H3.w, 16.0;

MADX H5, H3.w, H4.w, H5;		// diffuse * base + specular

MULX H5, H5, H0.w;				// all * attenuation
MULX o[COLH], H5, f[COL0];

END
