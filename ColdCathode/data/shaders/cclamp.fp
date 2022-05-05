!!ARBfp1.0

PARAM radius = program.local[0];

TEMP dist;
DP3 dist.x, fragment.texcoord[0], fragment.texcoord[1];
ADD dist.x, dist.x, fragment.texcoord[1].w;
MAX dist.x, dist.x, -dist.x;

DP3 dist.y, fragment.texcoord[0], fragment.texcoord[2];
ADD dist.y, dist.y, fragment.texcoord[2].w;
MAX dist.y, -dist.y, 0;

DP3 dist.z, fragment.texcoord[0], fragment.texcoord[3];
ADD dist.z, dist.z, fragment.texcoord[3].w;
MAX dist.z, dist.z, 0;

DP3 dist.w, dist, dist;
RSQ dist.w, dist.w;
RCP dist, dist.w;

MUL dist, dist, radius.x;

TEX result.color, dist, texture[0], 1D;

END
