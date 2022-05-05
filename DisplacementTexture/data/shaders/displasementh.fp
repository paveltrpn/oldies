!!ARBfp1.0

TEMP dir, displacement, d0, d1;

TEX dir, fragment.texcoord[1], texture[2], CUBE;

MOV displacement, fragment.texcoord[0];

MOV displacement.z, dir.x;
TEX d0, displacement, texture[3], 3D;
MAD d0, d0, 2.0, -1.0;

MOV displacement.z, dir.y;
TEX d1, displacement, texture[3], 3D;
MAD d1, d1, 2.0, -1.0;

LRP displacement, dir.z, d1, d0;
ADD displacement, displacement.xwxx, fragment.texcoord[0];

TEMP normal;
TEX normal, displacement, texture[1], 2D;
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
TEX base, displacement, texture[0], 2D;

TEMP attenuation;
SUB attenuation, fragment.texcoord[3], fragment.texcoord[4];
DP3 attenuation.w, attenuation, attenuation;
RSQ attenuation.w, attenuation.w;
RCP attenuation.w, attenuation.w;
MAD_SAT attenuation, attenuation.w, -fragment.texcoord[4].w, 1.0;

MUL diffuse, diffuse, attenuation;

TEMP tmp, shadow;
TEX tmp, dir, texture[4], CUBE;
MOV displacement.z, tmp.x;
TEX shadow, displacement, texture[5], 3D;

SUB_SAT shadow, tmp.w, shadow;
MUL_SAT shadow, shadow, 8;
ADD_SAT shadow, shadow, 0.45;

MUL diffuse, diffuse, shadow;

MUL result.color, base, diffuse;

END
