!!ARBfp1.0

PARAM color = program.local[0];	# (color,attenuation)

TEMP tmp, normal;
TEX tmp, fragment.texcoord[6], texture[0], 2D;
MAD tmp, tmp, 2.0, -1.0;
DP3 tmp.w, tmp, tmp;
RSQ tmp.w, tmp.w;
MUL tmp, tmp, tmp.w;

DP3 normal.x, fragment.texcoord[3], tmp;
DP3 normal.y, fragment.texcoord[4], tmp;
DP3 normal.z, fragment.texcoord[5], tmp;

TEMP point;
DP3 point.w, fragment.texcoord[0], fragment.texcoord[2];
SUB point.w, point.w, fragment.texcoord[1].w;
MUL_SAT point.w, point.w, fragment.texcoord[2].w;
MAD point, fragment.texcoord[2], point.w, fragment.texcoord[1];

TEMP dir, diffuse;
SUB dir, point, fragment.texcoord[0];
DP3 dir.w, dir, dir;
RSQ dir.w, dir.w;
MUL dir.xyz, dir, dir.w;

DP3_SAT diffuse, dir, normal;

MUL dir.w, dir.w, color.w;				# attenuation
MUL diffuse, diffuse, dir.w;
MUL result.color, diffuse, color;

END
