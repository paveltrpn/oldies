!!ARBfp1.0

PARAM param = { 2.0, -1.0, 32.0, 0.0 };
PARAM color = { 1.0, 0.8, 0.0, 1.0 };

TEMP base, dot3, height, diffuse, specular;

TEX dot3, fragment.texcoord[0], texture[1], 3D;
TEX height, fragment.texcoord[0], texture[2], 3D;

MAD dot3, dot3, param.x, param.y;
MUL dot3.z, dot3.z, 0.5;
DP3 dot3.w, dot3, dot3;
RSQ dot3.w, dot3.w;
MUL dot3.xyz, dot3, dot3.w;

DP3_SAT diffuse, fragment.texcoord[1], dot3;
DP3_SAT specular, fragment.texcoord[2], dot3;
POW specular, specular.x, param.z;

TEX base, height, texture[0], 1D;

MUL height, height, 4;
MUL specular, specular, height;

MUL specular, specular, color;

MAD result.color, base, diffuse, specular;

END
