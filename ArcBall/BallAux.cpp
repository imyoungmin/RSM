/***** BallAux.c *****/
#include "BallAux.h"

Quat qOne = {0,	0, 0, 1};

/* Return quaternion product qL	* qR.  Note: order is important!
 * To combine rotations, use the product Mul(qSecond, qFirst),
 * which gives the effect of rotating by qFirst	then qSecond. */
Quat Qt_Mul(Quat qL, Quat qR)
{
    Quat qq;
    qq.w = qL.w*qR.w - qL.x*qR.x - qL.y*qR.y - qL.z*qR.z;
    qq.x = qL.w*qR.x + qL.x*qR.w + qL.y*qR.z - qL.z*qR.y;
    qq.y = qL.w*qR.y + qL.y*qR.w + qL.z*qR.x - qL.x*qR.z;
    qq.z = qL.w*qR.z + qL.z*qR.w + qL.x*qR.y - qL.y*qR.x;
    return (qq);
}

Quat *Qt_ToQuat(HMatrix m, Quat *out)
{
	double e[4];
    double tr,s;
    int	i,j,k;

    tr = m[0][0]+m[1][1]+m[2][2];
    if (tr > 0.0) {
	s = sqrt(tr+1.0);
	e[3] = s*0.5;
	s = 0.5/s;

	e[0] = (m[1][2]-m[2][1])*s;
	e[1] = (m[2][0]-m[0][2])*s;
	e[2] = (m[0][1]-m[1][0])*s;
    }
    else {
	i = 0;
	if (m[1][1] > m[0][0]) i = 1;
	if (m[2][2] > m[i][i]) i = 2;
	j = (i+1) % 3; k = (j+1) % 3;

	s = sqrt((m[i][i]-(m[j][j]+m[k][k])) + 1.0);
	e[i] = s*0.5;
	s = 0.5/s;
	e[j] = (m[i][j]	+ m[j][i])*s;
	e[k] = (m[i][k]	+ m[k][i])*s;
	e[3] = (m[j][k]	- m[k][j])*s;
    }

	out->x = (float)e[0];
	out->y = (float)e[1];
	out->z = (float)e[2];
	out->w = (float)e[3];

    return (out);

}

/* Construct rotation matrix from (possibly non-unit) quaternion.
 * Assumes matrix is used to multiply column vector on the left:
 * vnew	= mat vold.  Works correctly for right-handed coordinate system
 * and right-handed rotations. */
void Qt_ToMatrix(Quat q, HMatrix out)
{
    double Nq =	q.x*q.x	+ q.y*q.y + q.z*q.z + q.w*q.w;
    double s = (Nq > 0.0) ? (2.0 / Nq) : 0.0;
    double xs =	q.x*s,	      ys = q.y*s,	  zs = q.z*s;
    double wx =	q.w*xs,	      wy = q.w*ys,	  wz = q.w*zs;
    double xx =	q.x*xs,	      xy = q.x*ys,	  xz = q.x*zs;
    double yy =	q.y*ys,	      yz = q.y*zs,	  zz = q.z*zs;
    out[X][X] =	(float)1.0 - (float)(yy	+ zz); out[Y][X] = (float)(xy +	wz); out[Z][X] = (float)(xz - wy);
    out[X][Y] =	(float)(xy - wz); out[Y][Y] = (float)1.0 - (float)(xx +	zz); out[Z][Y] = (float)(yz + wx);
    out[X][Z] =	(float)(xz + wy); out[Y][Z] = (float)(yz - wx);	out[Z][Z] = (float)1.0 - (float)(xx + yy);
    out[X][W] =	out[Y][W] = out[Z][W] =	out[W][X] = out[W][Y] =	out[W][Z] = 0.0;
    out[W][W] =	1.0;
}

/* Return conjugate of quaternion. */
Quat Qt_Conj(Quat q)
{
    Quat qq;
    qq.x = -q.x; qq.y =	-q.y; qq.z = -q.z; qq.w	= q.w;
    return (qq);
}

/* Return vector formed	from components	*/
HVect V3_(float	x, float y, float z)
{
    HVect v;
    v.x	= x; v.y = y; v.z = z; v.w = 0;
    return (v);
}

/* Return norm of v, defined as	sum of squares of components */
float V3_Norm(HVect v)
{
    return ( v.x*v.x + v.y*v.y + v.z*v.z );
}

/* Return unit magnitude vector	in direction of	v */
HVect V3_Unit(HVect v)
{
    static HVect u = {0, 0, 0, 0};
    float vlen = (float)sqrt(V3_Norm(v));
    if (vlen !=	0.0) {
	u.x = v.x/vlen;	u.y = v.y/vlen;	u.z = v.z/vlen;
    }
    return (u);
}

/* Return version of v scaled by s */
HVect V3_Scale(HVect v,	float s)
{
    HVect u;
    u.x	= s*v.x; u.y = s*v.y; u.z = s*v.z; u.w = v.w;
    return (u);
}

/* Return negative of v	*/
HVect V3_Negate(HVect v)
{
    static HVect u = {0, 0, 0, 0};
    u.x	= -v.x;	u.y = -v.y; u.z	= -v.z;
    return (u);
}

/* Return sum of v1 and	v2 */
HVect V3_Add(HVect v1, HVect v2)
{
    static HVect v = {0, 0, 0, 0};
    v.x	= v1.x+v2.x; v.y = v1.y+v2.y; v.z = v1.z+v2.z;
    return (v);
}

/* Return difference of	v1 minus v2 */
HVect V3_Sub(HVect v1, HVect v2)
{
    static HVect v = {0, 0, 0, 0};
    v.x	= v1.x-v2.x; v.y = v1.y-v2.y; v.z = v1.z-v2.z;
    return (v);
}

/* Halve arc between unit vectors v0 and v1. */
HVect V3_Bisect(HVect v0, HVect	v1)
{
    HVect v = {0, 0, 0,	0};
    float Nv;
    v =	V3_Add(v0, v1);
    Nv = V3_Norm(v);
    if (Nv < 1.0e-5) {
	v = V3_(0, 0, 1);
    } else {
	v = V3_Scale(v,	(float)1/(float)sqrt(Nv));
    }
    return (v);
}

/* Return dot product of v1 and	v2 */
float V3_Dot(HVect v1, HVect v2)
{
    return (v1.x*v2.x +	v1.y*v2.y + v1.z*v2.z);
}


/* Return cross	product, v1 x v2 */
HVect V3_Cross(HVect v1, HVect v2)
{
    static HVect v = {0, 0, 0, 0};
    v.x	= v1.y*v2.z-v1.z*v2.y;
    v.y	= v1.z*v2.x-v1.x*v2.z;
    v.z	= v1.x*v2.y-v1.y*v2.x;
    return (v);
}
