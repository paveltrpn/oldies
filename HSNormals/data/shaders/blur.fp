!!ARBfp1.0

TEMP n0,n1,n2,n3;
TEX n0, fragment.texcoord[0], texture[0], RECT;
TEX n1, fragment.texcoord[1], texture[0], RECT;
TEX n2, fragment.texcoord[2], texture[0], RECT;
TEX n3, fragment.texcoord[3], texture[0], RECT;

MAD n0, n0, 2.0, -1.0;	# expand
MAD n1, n1, 2.0, -1.0;
MAD n2, n2, 2.0, -1.0;
MAD n3, n3, 2.0, -1.0;

TEMP res;
ADD res, n0, n1;
ADD res, res, n2;
ADD res, res, n3;

DP3 res.w, res, res;	# normalize
RSQ res.w, res.w;
MUL res, res, res.w;

MAD result.color, res, 0.5, 0.5;

END
