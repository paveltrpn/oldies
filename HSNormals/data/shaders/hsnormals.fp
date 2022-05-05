!!ARBfp1.0

TEMP n0, n1, n2;

TEX n0, fragment.texcoord[0], texture[0], RECT;
TEX n1, fragment.texcoord[1], texture[0], RECT;
TEX n2, fragment.texcoord[2], texture[0], RECT;

MAD n0, n0, 2.0, -1.0;	# expand
MAD n1, n1, 2.0, -1.0;
MAD n2, n2, 2.0, -1.0;

TEMP n;

MUL n, n0, fragment.texcoord[0].w;
MAD n, n1, fragment.texcoord[1].w, n;
MAD n, n2, fragment.texcoord[2].w, n;

DP3 n.w, n, n;
RSQ n.w, n.w;
MUL n, n, n.w;

MAD result.color, n, 0.5, 0.5;

END
