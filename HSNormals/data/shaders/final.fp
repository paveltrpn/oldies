!!ARBfp1.0

TEMP n;
TEX n, fragment.texcoord[0], texture[0], RECT;

MAD n, n, 2.0, -1.0;	# expand

TEMP tmp, reflect;
DP3 tmp.w, n, fragment.texcoord[1];
MUL tmp, n, tmp.w;
MAD reflect, tmp, -2.0, fragment.texcoord[1];	# reflect

TEMP c0, c1;
TEX c0, reflect, texture[2], CUBE;

DP3 tmp.x, n, fragment.texcoord[2];
DP3 tmp.y, n, fragment.texcoord[3];
MAD tmp, tmp, 0.5, 0.5;
TEX c1, tmp, texture[1], 2D;

SUB tmp, 1, c0;
MAD result.color, c1, tmp, c0;

END
