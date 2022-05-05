!!ARBfp1.0

TEMP normal;
TEX normal, fragment.texcoord[0], texture[1], 2D;
MAD normal, normal, 2.0, -1.0;
DP3 normal.w, normal, normal;
RSQ normal.w, normal.w;
MUL normal, normal, normal.w;

TEMP dir;
DP3 dir.w, fragment.texcoord[2], fragment.texcoord[2];
RSQ dir.w, dir.w;
MUL dir, fragment.texcoord[2], dir.w;

TEMP diffuse;
DP3 diffuse, normal, dir;

TEMP base;
TEX base, fragment.texcoord[0], texture[0], 2D;

TEMP attenuation;
SUB attenuation, fragment.texcoord[3], fragment.texcoord[4];
DP3 attenuation.w, attenuation, attenuation;
RSQ attenuation.w, attenuation.w;
RCP attenuation.w, attenuation.w;
MAD_SAT attenuation, attenuation.w, -fragment.texcoord[4].w, 1.0;

MUL diffuse, diffuse, attenuation;

MUL result.color, base, diffuse;

END
