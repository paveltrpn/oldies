!!ARBfp1.0

TEMP base, height;

TEX height, fragment.texcoord[0], texture[1], 3D;
TEX base, height, texture[0], 1D;

MOV result.color, base;

END
