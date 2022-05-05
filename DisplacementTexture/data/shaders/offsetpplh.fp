!!ARBfp1.0

TEMP height;
TEX height, fragment.texcoord[0], texture[2], 2D;

TEMP dir;
DP3 dir.w, fragment.texcoord[1], fragment.texcoord[1];
RSQ dir.w, dir.w;
MUL dir, fragment.texcoord[1], dir.w;

TEMP offset;
MAD offset, height, 0.07, -0.01;
MAD offset, offset, dir, fragment.texcoord[0];

TEMP normal;
TEX normal, offset, texture[1], 2D;
MAD normal, normal, 2.0, -1.0;
DP3 normal.w, normal, normal;
RSQ normal.w, normal.w;
MUL normal, normal, normal.w;

DP3 dir.w, fragment.texcoord[2], fragment.texcoord[2];
RSQ dir.w, dir.w;
MUL dir, fragment.texcoord[2], dir.w;

TEMP diffuse;
DP3 diffuse, normal, dir;

TEMP base;
TEX base, offset, texture[0], 2D;

TEMP attenuation;
SUB attenuation, fragment.texcoord[3], fragment.texcoord[4];
DP3 attenuation.w, attenuation, attenuation;
RSQ attenuation.w, attenuation.w;
RCP attenuation.w, attenuation.w;
MAD_SAT attenuation, attenuation.w, -fragment.texcoord[4].w, 1.0;

MUL diffuse, diffuse, attenuation;

TEMP tmp, shadow;
TEX tmp, dir, texture[4], CUBE;
MOV offset.z, tmp.x;
TEX shadow, offset, texture[5], 3D;

SUB_SAT shadow, tmp.w, shadow;
MUL_SAT shadow, shadow, 8;
ADD_SAT shadow, shadow, 0.45;

MUL diffuse, diffuse, shadow;

MUL result.color, base, diffuse;

END