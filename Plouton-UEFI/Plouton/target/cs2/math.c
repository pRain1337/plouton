/*++
*
* Source file for memory related functions
*
--*/

// our includes
#include "math.h"

/*
* Function:  QVectorMul
* --------------------
*  Multiplies the given vector with the given fix32_t value.
*
*  vector:			Vector to multiply.
*  mul:				Value to multiply the vector with.
*
*  returns:	Vector with the multiplied positions.
*
*/
QVector QVectorMul(QVector vector, fix32_t mul)
{
	// Define a temporary vector to hold the result
	QVector tempVector;

	// Now multiply each value in the vector with the given fix32_t value
	tempVector.x = fix32_mul(vector.x, mul);
	tempVector.y = fix32_mul(vector.y, mul);
	tempVector.z = fix32_mul(vector.z, mul);

	// Return the temporary vector
	return tempVector;
}

/*
* Function:  QVectorDiv
* --------------------
*  Divides the values in the given vector with the given fix32_t value.
*
*  vector:			Vector to divide.
*  div:				Value to divide the vector with.
*
*  returns:	Vector with the divided positions.
*
*/
QVector QVectorDiv(QVector vector, fix32_t div)
{
	// Define a temporary vector to hold the result
	QVector tempVector;

	// Now divide each value in the vector with the given fix32_t value
	tempVector.x = fix32_div(vector.x, div);
	tempVector.y = fix32_div(vector.y, div);
	tempVector.z = fix32_div(vector.z, div);

	// Return the temporary vector
	return tempVector;
}

/*
* Function:  QVectorSub
* --------------------
*  Subtracts the values from the second vector from the first vector and returns the result.
*
*  vector1:			Vector to substract from.
*  vector2:			Vector to substract.
*
*  returns:	Vector with the substracted values.
*
*/
QVector QVectorSub(QVector vector1, QVector vector2)
{
	// Define a temporary vector to hold the result
	QVector tempVector;

	// Now substract the value from the second vector from the value of the first vector
	tempVector.x = fix32_sub(vector1.x, vector2.x);
	tempVector.y = fix32_sub(vector1.y, vector2.y);
	tempVector.z = fix32_sub(vector1.z, vector2.z);

	// Return the temporary vector
	return tempVector;
}

/*
* Function:  QVectorAdd
* --------------------
*  Adds the values from both vectors together and returns the result.
*
*  vector1:			Vector to add.
*  vector2:			Vector to add.
*
*  returns:	Vector with the added values.
*
*/
QVector QVectorAdd(QVector vector1, QVector vector2)
{
	// Define a temporary vector to hold the result
	QVector tempVector;

	// Now add the values of both vectors together
	tempVector.x = fix32_add(vector1.x, vector2.x);
	tempVector.y = fix32_add(vector1.y, vector2.y);
	tempVector.z = fix32_add(vector1.z, vector2.z);

	// Return the temporary vector
	return tempVector;
}

/*
* Function:  QAngleNormalize
* --------------------
*  Normalizes the given angles in the Angle vector.
*  x between 90 & -90
*  y between 180 & -180
*  z is 0
*
*  vector:			fix32 Angle vector to normalize
*
*  returns:	Nothing
*
*/
VOID QAngleNormalize(PQAngle angle)
{
	// check if x angle is larger than 89
	if (fix32_larger(angle->x, fix32_89))
		angle->x = fix32_89; // Set it to 89

	// Check if x angle is smaller than -89
	if (fix32_less(angle->x, fix32_neg_89))
		angle->x = fix32_neg_89; // Set it to -89

	// Check if y angle is larger than 180
	while (fix32_larger(angle->y, fix32_180))
		angle->y = fix32_sub(angle->y, fix32_360); // Keep substracting 360 until it is less than 180

	// Check if y angle is less than -180
	while (fix32_less(angle->y, fix32_neg_180))
		angle->y = fix32_add(angle->y, fix32_360); // Keep adding 360 until it is more than -180

	// These checks are just a safety check
	// Check if y angle is larger than 180
	if (fix32_larger(angle->y, fix32_180))
		angle->y = fix32_180; // Set it to 180

	// Check if y angle is less than -180
	if (fix32_less(angle->y, fix32_neg_180))
		angle->y = fix32_neg_180; // Set it to -180

	// Always set z to zero
	angle->z = fix32_zero;

	// Return noting
	return;
}

/*
* Function:  QAngleMul
* --------------------
*  Multiplies the given angle with the given fix32_t value.
*
*  angle:			Angle to multiply.
*  mul:				Value to multiply the Angle with.
*
*  returns:	Angle with the multiplied positions.
*
*/
QAngle QAngleMul(QAngle angle, fix32_t mul)
{
	// Define a temporary angle to hold the result
	QAngle tempAngle;

	// Now multiply each value in the angle with the given fix32_t value
	tempAngle.x = fix32_mul(angle.x, mul);
	tempAngle.y = fix32_mul(angle.y, mul);
	tempAngle.z = fix32_mul(angle.z, mul);

	// Return the temporary angle
	return tempAngle;
}

/*
* Function:  QAngleDiv
* --------------------
*  Divides the values in the given Angle with the given fix32_t value.
*
*  angle:			Angle to divide.
*  div:				Value to divide the Angle with.
*
*  returns:	Angle with the divided positions.
*
*/
QAngle QAngleDiv(QAngle angle, fix32_t div)
{
	// Define a temporary angle to hold the result
	QAngle tempAngle;

	// Now divide each value in the angle with the given fix32_t value
	tempAngle.x = fix32_div(angle.x, div);
	tempAngle.y = fix32_div(angle.y, div);
	tempAngle.z = fix32_div(angle.z, div);

	// Return the temporary angle
	return tempAngle;
}

/*
* Function:  QAngleSub
* --------------------
*  Subtracts the values from the second Angle from the first Angle and returns the result.
*
*  angle1:			Angle to substract from.
*  angle2:			Angle to substract.
*
*  returns:	Angle with the substracted values.
*
*/
QAngle QAngleSub(QAngle angle1, QAngle angle2)
{
	// Define a temporary angle to hold the result
	QAngle tempAngle;

	// Now substract the value from the second angle from the value of the first angle
	tempAngle.x = fix32_sub(angle1.x, angle2.x);
	tempAngle.y = fix32_sub(angle1.y, angle2.y);
	tempAngle.z = fix32_sub(angle1.z, angle2.z);

	// Return the temporary angle
	return tempAngle;
}

/*
* Function:  QAngleAdd
* --------------------
*  Adds the values from both Angles together and returns the result.
*
*  angle1:			Angle to add.
*  angle2:			Angle to add.
*
*  returns:	Angle with the added values.
*
*/
QAngle QAngleAdd(QAngle angle1, QAngle angle2)
{
	// Define a temporary angle to hold the result
	QAngle tempAngle;

	// Now add the values of both angles together
	tempAngle.x = fix32_add(angle1.x, angle2.x);
	tempAngle.y = fix32_add(angle1.y, angle2.y);
	tempAngle.z = fix32_add(angle1.z, angle2.z);

	// Return the temporary angle
	return tempAngle;
}

/*
* Function:  FloatVectorToFixedVector
* --------------------
*  Converts a given float vector (4 byte arrays for each value) into a fix32_t vector we can use.
*
*  vector:			Pointer to an Vector which contains the float data (4 byte arrays for each value).
*
*  returns:	QVector with fix32_t values we can use
*
*/
QVector FloatVectorToFixedVector(PVector vector)
{
	// Define a temporary value for the new vector
	QVector newVector;

	// Convert each value to fix32_t and save it in the new QVector
	newVector.x = fix32_from_float(&vector->x[0]);
	newVector.y = fix32_from_float(&vector->y[0]);
	newVector.z = fix32_from_float(&vector->z[0]);

	// Return the value
	return newVector;
}

/*
* Function:  Length2D
* --------------------
*  Calculates the 2 dimensional lengt of the given QVector.
*
*  vector:			QVector to calculate the length of.
*
*  returns:	fix32_t value of the length
*
*/
fix32_t Length2D(QVector vector)
{
	// Multiple the 2 dimensional values (x,y), add them together and get the square root and return it
	return fix32_sqrt(fix32_add(fix32_mul(vector.x, vector.x), fix32_mul(vector.y, vector.y)));
}

/*
* Function:  Length3D
* --------------------
*  Calculates the 3 dimensional lengt of the given QVector.
*
*  vector:			QVector to calculate the length of.
*
*  returns:	fix32_t value of the length
*
*/
fix32_t Length3D(QVector vector)
{
	// Multiply the 3 dimensional values (x,y,z) add them together and get the square root and return it
	return fix32_sqrt(fix32_add(fix32_add(fix32_mul(vector.x, vector.x), fix32_mul(vector.y, vector.y)), fix32_mul(vector.z, vector.z)));
}

/*
* Function:  QDistanceTo
* --------------------
*  Calculates the 3 dimensional distance between two QVectors and returns it.
*
*  from:			QVector position that is the source.
*  to:				QVector position that is the destination.
*
*  returns:	Distance between the two QVectors.
*
*/
fix32_t	QDistanceTo(QVector from, QVector to)
{
	// Get the delta by substracting the second QVector from the first
	QVector delta = QVectorSub(from, to);

	// Now calculate the length of the delta and return it
	return Length3D(delta);
}

/*
* Function:  VectorAngles
* --------------------
*  This function converts the input forward QVector into angles and directly writes them in the passed pointer.
*
*  forward:			QVector input for the calculation.
*  angles:			QAngle output of the calculation.
*
*  returns:	Nothing
*
*/
VOID VectorAngles(QVector forward, PQAngle angles)
{
	// Check if the position is different than zero
	if (forward.y == fix32_zero && forward.x == fix32_zero)
	{
		angles->x = (fix32_larger(forward.z, fix32_zero)) ? fix32_270 : fix32_90;  // Pitch (up/down)
		angles->y = fix32_zero;   //yaw left/rights
	}
	else
	{
		fix32_t xAtan = fix32_atan2(fix32_mul(forward.z, fix32_neg_one), Length2D(forward));
		fix32_t yAtan = fix32_atan2(forward.y, forward.x);
		angles->x = fix32_mul(xAtan, fix32_div(fix32_neg_180, fix32_pi));
		angles->y = fix32_mul(yAtan, fix32_div(fix32_180, fix32_pi));

		if (fix32_larger(angles->y, fix32_90)) // TRUE if y is larger than 90
		{
			angles->y = fix32_sub(angles->y, fix32_180);
		}
		else if (fix32_less(angles->y, fix32_90)) // TRUE if y is less than 90
		{
			angles->y = fix32_add(angles->y, fix32_180);
		}
		else if (fix32_cmp(angles->y, fix32_90)) // TRUE if y is 90
		{
			angles->y = fix32_zero;
		}
	}

	// Z is always zero
	angles->z = fix32_zero;

	// We return nothing
	return;
}

/*
* Function:  CalcAngle
* --------------------
*  This function calculates the angles which are required to change so the source angles would represent the destination angles.
*
*  source:			QVector of the current position.
*  destination:		QVector where we want to look at.
*
*  returns:	Angles that are required to change to look at the destination
*
*/
QAngle CalcAngle(QVector source, QVector destination)
{
	// Define a temporary angles variable
	QAngle angles;

	// Calculate the delta between source and destination
	QVector delta = QVectorSub(source, destination);

	// Convert the delta into angles
	VectorAngles(delta, &angles);

	// Normalize them
	QAngleNormalize(&angles);

	// Return the angles
	return angles;
}

/*
* Function:  SinCos
* --------------------
*  Calculates the sin and cos based on the given radians..
*
*  radians:			Input fix32_t radians to calculate the sin and cos of.
*  sine:			Output fix32_t of the calculated sin.
*  cosine:			Output fix32_t of the calculated cos.
*
*  returns:	Nothing
*
*/
VOID SinCos(fix32_t radians, fix32_t* sine, fix32_t* cosine)
{
	// calculate the sin of the radians and directly write it into the parameter
	*sine = fix32_sin(radians);
	
	// calculate the cos of the radians and directly write it into the parameter
	*cosine = fix32_cos(radians);

	// Return nothing
	return;
}

/*
* Function:  DEG2RAD
* --------------------
*  Converts the given degree into a radians and returns it.
*
*  deg:				fix32_t of the degree.
*
*  returns:	The calculated radian as fix32_t
*
*/
fix32_t DEG2RAD(fix32_t deg)
{
	// TO-DO 06.10.2024 - Hardcode PI / 180
	
	// Multiple the degree with PI divided by 180 and return it
	return fix32_mul(deg, fix32_div(fix32_pi, fix32_180));
}

/*
* Function:  AngleVectors
* --------------------
*  This function converts the input angles into a vector and directly writes it into a parameter.
*
*  angles:			Input QAngles that will be converted into QVector.
*  forward:			Output QVector based on the angles.
*
*  returns:	Nothing
*
*/
VOID AngleVectors(PQAngle angles, PQVector forward)
{
	// Some variables used for calculation
	fix32_t sp, sy, cp, cy;

	// Convert the angle values into a radians and then calculate the sin and cos of them
	SinCos(DEG2RAD(angles->y), &sy, &cy);
	SinCos(DEG2RAD(angles->x), &sp, &cp);

	// Based on the sin and cos, calculate the position and write it directly into the parameter
	forward->x = fix32_mul(cp, cy);
	forward->y = fix32_mul(cp, sy);
	forward->z = fix32_mul(sp, fix32_neg_one);

	// Return nothing
	return;
}

/*
* Function:  GetScaledFov
* --------------------
*  Calculates a FOV based on the current angles, angles required to change and distance to the position.
*
*  viewangles:		QAngles where we are currently aiming.
*  aimangles:		QAngles where we should aim at.
*  distance:		fix32_t distance to the target.
*
*  returns:	The FOV value of these angles
*
*/
fix32_t GetScaledFov(QAngle viewangles, QAngle aimangles, fix32_t distance)
{
	// Create some temporary variables to hold the result
	QVector vecCurrentViewPosition, vecDestinationViewPosition;

	// convert our current angles into the current view position
	AngleVectors(&viewangles, &vecCurrentViewPosition);

	// Multiply this position by the distance
	vecCurrentViewPosition = QVectorMul(vecCurrentViewPosition, distance);

	// Convert the aim angles to the destination view position
	AngleVectors(&aimangles, &vecDestinationViewPosition);

	// Multiply this position by the distance
	vecDestinationViewPosition = QVectorMul(vecDestinationViewPosition, distance);

	// Now return the distance between those two positions
	return QDistanceTo(vecCurrentViewPosition, vecDestinationViewPosition);
}

/*
* Function:  SmoothAngle
* --------------------
*  This function smooths the given angles based on the percent added so we have less flicky movement.
*
*  source:			QAngle where we are currently aiming at.
*  destination:		QAngle we should aim at.
*  percent:			Percent that should be applied to the angles (the higher, the smaller movements).
*
*  returns: Smoothed QAngles to aim at
*
*/
QAngle SmoothAngle(QAngle source, QAngle destination, fix32_t percent)
{
	// Calculate the delta (movement required)
	QAngle angDelta = QAngleSub(destination, source);

	// Normalize it to be sure
	QAngleNormalize(&angDelta);

	// Now calculate the smoothed angles by multiplying the delta angles with (percent / 100) and adding the source angles again
	QAngle smoothAimPoint = QAngleAdd(source, QAngleMul(angDelta, fix32_div(percent, fix32_100)));

	// Normalize it to be sure
	QAngleNormalize(&smoothAimPoint);

	// Return the smoothed QAngle
	return smoothAimPoint;
}

/*
* Function:  DotProduct
* --------------------
*  This function calculates the DOT product of the given QVector and fix32_t array and returns it.
*
*  first:			QVector for the dot product.
*  second:			fix32_t* as the input is not a QVector but a Matrix.
*
*  returns:	The Dot product of the values.
*
*/
fix32_t DotProduct(QVector first, fix32_t* second)
{
	// Multiply the values of both inputs together and calculate the sum of them and then return it
	return fix32_add(fix32_mul(first.x, second[0]), fix32_add(fix32_mul(first.y, second[1]), fix32_mul(first.z, second[2])));
}

/*
* Function:  VectorTransform
* --------------------
*  Transform the given vector of the hitbox into a vector position based on the matrix.
*
*  in1:				QVector position in the hitbox.
*  in2:				matrix3x4 matrix of the hitbox to calculate.
*
*  returns:	Calculated position of the hitbox
*
*/
QVector VectorTransform(QVector in1, matrix3x4 in2)
{
	// Define a temporary vector variable
	QVector out;

	// Calculate the position by calculating the Dot product of the vector with the matrix and adding the origin back onto it
	out.x = fix32_add(DotProduct(in1, (fix32_t*)&in2.m_flMatVal[0]), in2.m_flMatVal[0][3]);
	out.y = fix32_add(DotProduct(in1, (fix32_t*)&in2.m_flMatVal[1]), in2.m_flMatVal[1][3]);
	out.z = fix32_add(DotProduct(in1, (fix32_t*)&in2.m_flMatVal[2]), in2.m_flMatVal[2][3]);

	// Return the calculated position
	return out;
}

/*
* Function:  GetHitboxCenter
* --------------------
*  Calculate the center of a given hitbox based on the minimum and maximum position of the bone in the hitbox and return it.
*
*  bonedata:		CTransform structure of the hitbox that was read from the game.
*  bone_min:		Minimum position of the bone.
*  bone_max:		Maximum position of the bone.
*
*  returns:	Calculated center position in the hitbox
*
*/
QVector GetHitboxCenter(CTransform bonedata, QVector bone_min, QVector bone_max)
{
	// Calculate the matrix of the hitbox using the rotation and origin
	matrix3x4 matrix;
	matrix.m_flMatVal[0][0] = fix32_sub(fix32_one, fix32_sub(fix32_mul(fix32_two, fix32_mul(bonedata.rotation.y, bonedata.rotation.y)), fix32_mul(fix32_two, fix32_mul(bonedata.rotation.z, bonedata.rotation.z))));
	matrix.m_flMatVal[1][0] = fix32_add(fix32_mul(fix32_two, fix32_mul(bonedata.rotation.x, bonedata.rotation.y)), fix32_mul(fix32_two, fix32_mul(bonedata.rotation.r, bonedata.rotation.z)));
	matrix.m_flMatVal[2][0] = fix32_sub(fix32_mul(fix32_two, fix32_mul(bonedata.rotation.x, bonedata.rotation.z)), fix32_mul(fix32_two, fix32_mul(bonedata.rotation.r, bonedata.rotation.y)));

	matrix.m_flMatVal[0][1] = fix32_sub(fix32_mul(fix32_two, fix32_mul(bonedata.rotation.x, bonedata.rotation.y)), fix32_mul(fix32_two, fix32_mul(bonedata.rotation.r, bonedata.rotation.z)));
	matrix.m_flMatVal[1][1] = fix32_sub(fix32_one, fix32_sub(fix32_mul(fix32_two, fix32_mul(bonedata.rotation.x, bonedata.rotation.x)), fix32_mul(fix32_two, fix32_mul(bonedata.rotation.z, bonedata.rotation.z))));
	matrix.m_flMatVal[2][1] = fix32_add(fix32_mul(fix32_two, fix32_mul(bonedata.rotation.y, bonedata.rotation.z)), fix32_mul(fix32_two, fix32_mul(bonedata.rotation.r, bonedata.rotation.x)));

	matrix.m_flMatVal[0][2] = fix32_add(fix32_mul(fix32_two, fix32_mul(bonedata.rotation.x, bonedata.rotation.z)), fix32_mul(fix32_two, fix32_mul(bonedata.rotation.r, bonedata.rotation.y)));
	matrix.m_flMatVal[1][2] = fix32_sub(fix32_mul(fix32_two, fix32_mul(bonedata.rotation.y, bonedata.rotation.z)), fix32_mul(fix32_two, fix32_mul(bonedata.rotation.r, bonedata.rotation.x)));
	matrix.m_flMatVal[2][2] = fix32_sub(fix32_one, fix32_sub(fix32_mul(fix32_two, fix32_mul(bonedata.rotation.x, bonedata.rotation.x)), fix32_mul(fix32_two, fix32_mul(bonedata.rotation.y, bonedata.rotation.y))));

	matrix.m_flMatVal[0][3] = bonedata.origin.x;
	matrix.m_flMatVal[1][3] = bonedata.origin.y;
	matrix.m_flMatVal[2][3] = bonedata.origin.z;

	// Calculate the mnimum and maximum positions in the hitbox based on the matrix
	QVector omin = VectorTransform(bone_min, matrix);
	QVector omax = VectorTransform(bone_max, matrix);

	// Now get the point in the center
	QVector retn = QVectorMul(QVectorAdd(omin, omax), fix32_05);

	// Return the point in the center
	return retn;
}