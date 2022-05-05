!!ARBfp1.0

# x = far + near
# y = near - far
# z = 2 * far * near
# w = 1 / shadow_end
PARAM zbuffer = program.env[0];

TEMP temp, depth_0, depth_1;

TXP depth_0, fragment.texcoord[0], texture[0], 2D;
TXP depth_1, fragment.texcoord[0], texture[1], 2D;

SUB temp, depth_0, depth_1;
SUB temp.x, temp.x, 0.0001;
KIL temp.x;

# 2 * far * near / (far + near - (far - near) * (2.0 * depth -1.0))

MAD depth_0, depth_0, 2, -1;
MAD depth_0, depth_0, zbuffer.y, zbuffer.x;
RCP depth_0, depth_0.x;
MUL depth_0, depth_0, zbuffer.z;

MAD depth_1, depth_1, 2, -1;
MAD depth_1, depth_1, zbuffer.y, zbuffer.x;
RCP depth_1, depth_1.x;
MUL depth_1, depth_1, zbuffer.z;

SUB temp, depth_0, depth_1;

MUL_SAT result.color, temp, zbuffer.w;

END
