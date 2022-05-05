<vertex>

void main() {
	
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	
	float dx = 1.0 / 128.0;
	float dy = 1.0 / 128.0;
	
	vec2 texcoord = gl_MultiTexCoord0.xy;
	gl_TexCoord[0].xy = texcoord;
	
	gl_TexCoord[1].xy = texcoord + vec2(dx,0.0);
	gl_TexCoord[1].zw = texcoord + vec2(-dx,0.0);
	
	gl_TexCoord[2].xy = texcoord + vec2(0.0,dy);
	gl_TexCoord[2].zw = texcoord + vec2(0.0,-dy);
	
	gl_TexCoord[3].xy = texcoord + vec2(dx,dy);
	gl_TexCoord[3].zw = texcoord + vec2(-dx,dy);
	
	gl_TexCoord[4].xy = texcoord + vec2(dx,-dy);
	gl_TexCoord[4].zw = texcoord + vec2(-dx,-dy);
}

<fragment>

uniform sampler2D s_texture_0;

void main() {
	
	vec4 c0 = texture2D(s_texture_0,gl_TexCoord[0].xy);
	
	vec4 c1 = texture2D(s_texture_0,gl_TexCoord[1].xy);
	vec4 c2 = texture2D(s_texture_0,gl_TexCoord[1].zw);
	vec4 c3 = texture2D(s_texture_0,gl_TexCoord[2].xy);
	vec4 c4 = texture2D(s_texture_0,gl_TexCoord[2].zw);
	vec4 c5 = texture2D(s_texture_0,gl_TexCoord[3].xy);
	vec4 c6 = texture2D(s_texture_0,gl_TexCoord[3].zw);
	vec4 c7 = texture2D(s_texture_0,gl_TexCoord[4].xy);
	vec4 c8 = texture2D(s_texture_0,gl_TexCoord[4].zw);
	
	if(c0.w > 0.1) gl_FragColor = c0;
	else if(c1.w > 0.1) gl_FragColor = c1;
	else if(c2.w > 0.1) gl_FragColor = c2;
	else if(c3.w > 0.1) gl_FragColor = c3;
	else if(c4.w > 0.1) gl_FragColor = c4;
	else if(c5.w > 0.1) gl_FragColor = c5;
	else if(c6.w > 0.1) gl_FragColor = c6;
	else if(c7.w > 0.1) gl_FragColor = c7;
	else if(c8.w > 0.1) gl_FragColor = c8;
}
