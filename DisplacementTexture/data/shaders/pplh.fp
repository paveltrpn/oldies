!!ARBfp1.0

TEMP normal;
TEX normal, fragment.texcoord[0], texture[1], 2D;		# normal
MAD normal, normal, 2.0, -1.0;
DP3 normal.w, normal, normal;
RSQ normal.w, normal.w;
MUL normal, normal, normal.w;

TEMP dir;
DP3 dir.w, fragment.texcoord[2], fragment.texcoord[2];	# direction to light in local coordinates
RSQ dir.w, dir.w;
MUL dir, fragment.texcoord[2], dir.w;

TEMP diffuse;
DP3_SAT diffuse, normal, dir;

TEMP base;
TEX base, fragment.texcoord[0], texture[0], 2D;

TEMP attenuation;
SUB attenuation, fragment.texcoord[3], fragment.texcoord[4];
DP3 attenuation.w, attenuation, attenuation;
RSQ attenuation.w, attenuation.w;
RCP attenuation.w, attenuation.w;
MAD_SAT attenuation, attenuation.w, -fragment.texcoord[4].w, 1.0;

MUL diffuse, diffuse, attenuation;

TEMP tmp, shadow;
TEX tmp, dir, texture[4], CUBE;
MOV shadow, fragment.texcoord[0];
MOV shadow.z, tmp.x;
TEX shadow, shadow, texture[5], 3D;

SUB_SAT shadow, tmp.w, shadow;
MUL_SAT shadow, shadow, 8;
ADD_SAT shadow, shadow, 0.45;

MUL diffuse, diffuse, shadow;

MUL result.color, base, diffuse;

END
