/***** Ball.h *****/
#ifndef	_H_Ball
#define	_H_Ball

#include "BallAux.h"
#include "BallMath.h"
#include <cmath>

typedef	enum AxisSet{NoAxes, CameraAxes, BodyAxes, OtherAxes, NSets} AxisSet;
typedef	float *ConstraintSet;
typedef	struct {
	HVect center;
	double radius;
	Quat qNow, qDown, qDrag;
	HVect vNow,	vDown, vFrom, vTo, vrFrom, vrTo;
	HMatrix mNow, mDown;
	int	dragging;
	ConstraintSet sets[NSets];
	int	setSizes[NSets];
	AxisSet axisSet;
	int	axisIndex;
	HMatrix userAxes;
} BallData;

/* Public routines */
void Ball_Init(BallData *ball,float *init_matrix=NULL);
void Ball_Place(BallData *ball, HVect center, double radius);
void Ball_Mouse(BallData *ball, HVect vNow);
void Ball_UseSet(BallData *ball, AxisSet axisSet);
void Ball_SetOtherAxes(BallData	*ball, HMatrix conAxis);
void Ball_Update(BallData *ball);
void Ball_Value(BallData *ball, HMatrix mNow);
void Ball_Quat(BallData *ball, float qNow[4]);
void Ball_BeginDrag(BallData *ball);
void Ball_BeginDragReset(BallData *ball);
void Ball_EndDrag(BallData *ball);
#endif
