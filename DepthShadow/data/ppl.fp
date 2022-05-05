!!ARBfp1.0

PARAM param = { 2.0, -1.0, 32.0, 0.0 };

TEMP base, dot3, gloss, diffuse, specular;

TEX base, fragment.texcoord[0], texture[0], 2D;
TEX dot3, fragment.texcoord[0], texture[1], 2D;
TEX gloss, fragment.texcoord[0], texture[2], 2D;

MAD dot3, dot3, param.x, param.y;
DP3 dot3.w, dot3, dot3;
RSQ dot3.w, dot3.w;
MUL dot3.xyz, dot3, dot3.w;

DP3_SAT diffuse, fragment.texcoord[1], dot3;

DP3_SAT specular, fragment.texcoord[2], dot3;

POW specular, specular.x, param.z;

MUL specular, specular, gloss;

MAD result.color, base, diffuse, specular;

END
