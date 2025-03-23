/*++
*
* Header file for memory related functions
*
--*/

#ifndef __aristoteles_math_cs_h__
#define __aristoteles_math_cs_h__

#include "../../floats/floatlib.h"

// Structures

// Vector which contains 4 byte arrays which represents floats
typedef struct _Vector {
	unsigned char x[4];
	unsigned char y[4];
	unsigned char z[4];
} Vector, * PVector;

// 4D Vector with fix32_t values that were converted from floats
typedef struct _QVector4D
{
	fix32_t x;
	fix32_t y;
	fix32_t z;
	fix32_t r;
} QVector4D, * PQVector4D;

// Alternative vector which contains fix32_t values that were converted from floats
typedef struct _QVector
{
	fix32_t x;
	fix32_t y;
	fix32_t z;
} QVector, * PQVector;

// Alternative angles which contains fix32_t values that were converted from floats
typedef struct _QAngle
{
	fix32_t x;
	fix32_t y;
	fix32_t z;
} QAngle, * PQAngle;

// Used to get the bone position
typedef struct _CTransform
{
	QVector origin;
	fix32_t scale;
	QVector4D rotation;
} CTransform, * PCTransform;

typedef struct _matrix3x4
{
	fix32_t m_flMatVal[3][4];
} matrix3x4, * Pmatrix3x4;


// Definitions

// Functions

// - General functions

/*
* Function:  QAngleNormalize
* --------------------
* This function normalizes the fix32_t angles in the given vector.
*/
VOID QAngleNormalize(PQAngle vector);

/*
* Function:  FloatVectorToFixedVector
* --------------------
* This function converts the raw float data into an usable QVector with fix32_t values.
*/
QVector FloatVectorToFixedVector(PVector vector);

// - QVector functions

/*
* Function:  QVectorMul
* --------------------
* This function multiples the values in the vector with the fix32_t value.
*/
QVector QVectorMul(QVector vector, fix32_t mul);

/*
* Function:  QVectorDiv
* --------------------
* This function divides the values in the vector with the fix32_t value.
*/
QVector QVectorDiv(QVector vector, fix32_t div);

/*
* Function:  QVectorSub
* --------------------
* This function substracts the second vector from the first vector.
*/
QVector QVectorSub(QVector vector1, QVector vector2);

/*
* Function:  QVectorAdd
* --------------------
* This function adds the two vectors together.
*/
QVector QVectorAdd(QVector vector1, QVector vector2);

// - QAngle functions

/*
* Function:  QAngleMul
* --------------------
* This function multiplies the values in the angle with the fix32_t value.
*/
QAngle QAngleMul(QAngle angle, fix32_t mul);

/*
* Function:  QAngleDiv
* --------------------
* This function divides the values in the angle with the fix32_t value.
*/
QAngle QAngleDiv(QAngle angle, fix32_t div);

/*
* Function:  QAngleSub
* --------------------
* This function substracts the second angle from the first angle.
*/
QAngle QAngleSub(QAngle angle1, QAngle angle2);

/*
* Function:  QAngleAdd
* --------------------
* This function adds the two angles together.
*/
QAngle QAngleAdd(QAngle angle1, QAngle angle2);

// - FOV functions

/*
* Function:  Length2D
* --------------------
* This function calculates the 2 dimensional length of a vector.
*/
fix32_t Length2D(QVector vector);

/*
* Function:  Length3D
* --------------------
* This function calculates the 3 dimensional length of a vector.
*/
fix32_t Length3D(QVector vector);

/*
* Function:  QDistanceTo
* --------------------
* This function calculates the distance between two 3 dimensional positions.
*/
fix32_t	QDistanceTo(QVector from, QVector to);

/*
* Function:  GetScaledFov
* --------------------
* This function calculates the FOV between two angles and the distance of the position.
*/
fix32_t GetScaledFov(QAngle viewangles, QAngle aimangles, fix32_t distance);

// - Transformation functions

/*
* Function:  AngleVectors
* --------------------
* This function converts the input angles into a vector.
*/
VOID AngleVectors(PQAngle angles, PQVector forward);

/*
* Function:  VectorAngles
* --------------------
* This function converts the input vector into an angle.
*/
VOID VectorAngles(QVector forward, PQAngle angles);

/*
* Function:  VectorTransform
* --------------------
* This function transforms the given vector of the hitbox into a vector position based on the matrix.
*/
QVector VectorTransform(QVector in1, matrix3x4 in2);

// - ViewAngles functions

/*
* Function:  CalcAngle
* --------------------
* This function calculates the required angle change from the source to aim at the destination.
*/
QAngle CalcAngle(QVector source, QVector destination);

/*
* Function:  SmoothAngle
* --------------------
* This function calculates the smoothed angle change from the source to aim at the destination.
*/
QAngle SmoothAngle(QAngle source, QAngle destination, fix32_t percent);

/*
* Function:  GetHitboxCenter
* --------------------
* This function calculates the center position in a hitbox based on the minimum and maximum position.
*/
QVector GetHitboxCenter(CTransform bonedata, QVector bone_min, QVector bone_max);

// - General math functions

/*
* Function:  SinCos
* --------------------
* This function calculates the sin and cos of a radians.
*/
VOID SinCos(fix32_t radians, fix32_t* sine, fix32_t* cosine);

/*
* Function:  DEG2RAD
* --------------------
* This function converts the given degree into a radians.
*/
fix32_t DEG2RAD(fix32_t deg);

/*
* Function:  DotProduct
* --------------------
* This function calculates the dot product based on two Vectors.
*/
fix32_t DotProduct(QVector first, fix32_t* second);

#endif