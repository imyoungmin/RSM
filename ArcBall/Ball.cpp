/***** Ball.c *****/
/* Ken Shoemake, 1993 */
/* Modified by Victor Ng, Jan. 1996 for	OpenGL */

#include "Ball.h"

#ifndef	WIN32
#ifndef	sinf
#define	sinf sin
#define	cosf cos
#endif
#endif

#define	FALSE 0
#define	TRUE 1

#define	LG_NSEGS 4
#define	NSEGS (1<<LG_NSEGS)

HMatrix	mId = {{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};
float otherAxis[][4] = {{(float)-0.48, (float)0.80, (float)0.36, (float)1.0}};

/* Establish reasonable	initial	values for controller. */
void Ball_Init(BallData	*ball, float *init_matrix)
{
    int	i;
    ball->center = qOne;
    ball->radius = 1.0;
    ball->vDown	= ball->vNow = qOne;
    ball->qDown	= ball->qNow = qOne;
    for	(i=15; i>=0; i--) {
		if (init_matrix	== NULL) 
			((float *)ball->mNow)[i] = ((float *)ball->mDown)[i]	= ((float *)mId)[i];
		else 
			((float *)ball->mNow)[i] = ((float *)ball->mDown)[i]	= init_matrix[i];
		
	}

	// We must also set the qNow quaternion to correspond with the 
	// given initial rotation matrix.
	if (init_matrix) {
		Qt_ToQuat(ball->mNow,&(ball->qNow));
		ball->qDown = ball->qNow;
	}

    ball->axisSet = NoAxes;
    ball->sets[CameraAxes] = mId[X]; ball->setSizes[CameraAxes]	= 3;
    ball->sets[BodyAxes] = ball->mDown[X]; ball->setSizes[BodyAxes] = 3;
    ball->sets[OtherAxes] = otherAxis[X]; ball->setSizes[OtherAxes] = 1;
}

/* Set the center and size of the controller. */
void Ball_Place(BallData *ball,	HVect center, double radius)
{
    ball->center = center;
    ball->radius = radius;
}

/* Incorporate new mouse position. */
void Ball_Mouse(BallData *ball,	HVect vNow)
{
    ball->vNow = vNow;
}

/* Choose a constraint set, or none. */
void Ball_UseSet(BallData *ball, AxisSet axisSet)
{
    if (!ball->dragging) ball->axisSet = axisSet;
}

/* Set the OtherAxes for constraints. */
void Ball_SetOtherAxes(BallData	*ball, HMatrix conAxis)
{
	for (int i=0; i < 4; i++)
	for (int j=0; j < 4; j++)
		ball->userAxes[i][j] = conAxis[i][j];
	
	ball->sets[OtherAxes] = ball->userAxes[X];
	ball->setSizes[OtherAxes] = 3;
}

/* Using vDown,	vNow, dragging,	and axisSet, compute rotation etc. */
void Ball_Update(BallData *ball)
{
    int	setSize	= ball->setSizes[ball->axisSet];
    HVect *set = (HVect	*)(ball->sets[ball->axisSet]);
    ball->vFrom	= MouseOnSphere(ball->vDown, ball->center, ball->radius);
    ball->vTo =	MouseOnSphere(ball->vNow, ball->center,	ball->radius);
    if (ball->dragging)	{
	if (ball->axisSet!=NoAxes) {
	    ball->vFrom	= ConstrainToAxis(ball->vFrom, set[ball->axisIndex]);
	    ball->vTo =	ConstrainToAxis(ball->vTo, set[ball->axisIndex]);
	}
	ball->qDrag = Qt_FromBallPoints(ball->vFrom, ball->vTo);
	ball->qNow = Qt_Mul(ball->qDrag, ball->qDown);
    } else {
	if (ball->axisSet!=NoAxes) {
	    ball->axisIndex = NearestConstraintAxis(ball->vTo, set, setSize);
	}
    }
    Qt_ToBallPoints(ball->qDown, &ball->vrFrom,	&ball->vrTo);
    Qt_ToMatrix(Qt_Conj(ball->qNow), ball->mNow); /* Gives transpose for GL. */
}

/* Return rotation matrix defined by controller	use. */
void Ball_Value(BallData *ball,	HMatrix	mNow)
{
    int	i;
    for	(i=15; i>=0; i--) ((float *)mNow)[i] = ((float *)ball->mNow)[i];
}

/* Return quaternion defined by controller use. */
void Ball_Quat(BallData *ball, float qNow[4])
{
	qNow[0] = ball->qNow.x;
	qNow[1] = ball->qNow.y;
	qNow[2] = ball->qNow.z;
	qNow[3] = ball->qNow.w;
}

/* Begin drag sequence.	*/
void Ball_BeginDrag(BallData *ball)
{
    ball->dragging = TRUE;
    ball->vDown	= ball->vNow;
}

/* Begin drag sequence.	*/
void Ball_BeginDragReset(BallData *ball)
{
    int	i;

    ball->dragging = TRUE;
    ball->vDown	= ball->vNow;

    // Set to Identity
    ball->qDown	= ball->qNow = qOne;
    for	(i=15; i>=0; i--)
       ((float *)ball->mNow)[i]	= ((float *)ball->mDown)[i] = ((float *)mId)[i];
}

/* Stop	drag sequence. */
void Ball_EndDrag(BallData *ball)
{
    int	i;
    ball->dragging = FALSE;
    ball->qDown	= ball->qNow;
    for	(i=15; i>=0; i--)
	((float	*)ball->mDown)[i] = ((float *)ball->mNow)[i];
}