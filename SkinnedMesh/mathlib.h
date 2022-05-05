/*  3d math lib
 *
 *      written by Alexander Zaprjagaev
 *      frustum@public.tsu.ru
 *      2:5005/93.15@FidoNet
 */

#ifndef __MATHLIB_H__
#define __MATHLIB_H__

#define PI 3.14159265358979323846
#define DEG2RAD (PI / 180.0)
#define RAD2DEG (180.0 / PI)

#define VectorSet(x,y,z,v) { (v)[0] = x; (v)[1] = y; (v)[2] = z; }
#define VectorClear(v) { (v)[0] = (v)[1] = (v)[2] = 0; }
#define VectorCopy(a,b) { (b)[0] = (a)[0]; (b)[1] = (a)[1]; (b)[2] = (a)[2]; }
#define VectorScale(a,b,c) { (c)[0] = (a)[0] * (b); (c)[1] = (a)[1] * (b); (c)[2] = (a)[2] * (b); }
#define VectorAdd(a,b,c) { (c)[0] = (a)[0] + (b)[0]; (c)[1] = (a)[1] + (b)[1]; (c)[2] = (a)[2] + (b)[2]; }
#define VectorSub(a,b,c) { (c)[0] = (a)[0] - (b)[0]; (c)[1] = (a)[1] - (b)[1]; (c)[2] = (a)[2] - (b)[2]; }
#define VectorDotProduct(a,b) ((a)[0] * (b)[0] + (a)[1] * (b)[1] + (a)[2] * (b)[2])

float VectorLength(const float *v);
float VectorNormalize(const float *v,float *out);
void VectorCrossProduct(const float *v1,const float *v2,float *out);
void VectorTransform(const float *v,const float *m,float *out);
void VectorTransformNormal(const float *v,const float *m,float *out);
void MatrixMultiply(const float *m1,const float *m2,float *out);
int MatrixInverse(const float *m,float *out);
void MatrixTranspose(const float *m,float *out);
void MatrixRotationX(float angle,float *out);
void MatrixRotationY(float angle,float *out);
void MatrixRotationZ(float angle,float *out);
void MatrixTranslate(float x,float y,float z,float *out);
void MatrixLookAt(const float *eye,const float *dir,const float *up,float *out);
void MatrixShadow(const float *plane,const float *light,float *out);
void QuaternionSet(float *dir,float angle,float *out);
void QuaternionCopy(float *q,float *out);
void QuaternionSlerp(float *q1,float *q2,float t,float *out);
void QuaternionMultiply(float *q1,float *q2,float *out);
void QuaternionToMatrix(float *q,float *out);

#endif /* __MATHLIB_H__ */
