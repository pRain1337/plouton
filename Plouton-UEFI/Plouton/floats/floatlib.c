/*++
*
* Source file for fixed point related functionalities
*
* For more general implementation, see https://github.com/jussihi/libfixmath64
*
--*/

// our includes
#include "floatlib.h"

/*
* Function:  clz
* --------------------
*  Counts the leading zeros (zero bits) of a value.
*
*  x:				Value to count the leading zeros of.
*
*  returns:	UINT8 containing the leading zeros
*
*/
static UINT8 clz(UINT64 x)
{
	UINT8 result = 0;
	if (x == 0)
		return 64;

	while (!(x & 0xF000000000000000))
	{
		result += 4;
		x <<= 4;
	}

	while (!(x & 0x8000000000000000))
	{
		result += 1;
		x <<= 1;
	}

	return result;
}

/*
* Function:  float2int
* --------------------
*  Converts the given float into a 32 byte signed integer.
*
*  value:			Pointer to 4 bytes that contain a float.
*
*  returns:	Converted 32 byte signed integer
*
*/
INT32 float2int(const void* value)
{
	const UINT32 INT32_MIN_POSITIVE = 2147483648; // -INT32_MIN

	IEEE745_float number = { .float_value = *(INT32*)value };

	const int shift = number.fields.exp_bias - IEEE745_FLOAT_BIAS - 23;

	// shift amount is greater then number of significant bits,
	// so integer part is always 0
	if (shift < -23)
	{
		return 0;
	}

	// shift amount is greater than 7 (31 - 24), so integer exceed 31 bits
	if (shift > 7)
	{
		return number.fields.sign ? INT32_MIN : INT32_MAX;
	}

	// shift amount in range [-23 .. 7]
	INT64 integer = number.fields.fraction | (1 << 23); // set implicit integer 1

	if (shift < 0)
		integer >>= -shift;
	else if (shift > 0)
		integer <<= shift;

	if (shift == 7)
	{
		// range checking is required only when number of significant bits
		// is exacly 31.
		if (number.fields.sign)
		{
			if (integer > INT32_MIN_POSITIVE)
			{
				return INT32_MIN;
			}
		}
		else
		{
			if (integer > INT32_MAX)
			{
				return INT32_MAX;
			}
		}
	}
	return (number.fields.sign) ? -integer : integer;
}

/*
* Function:  fix32_to_int
* --------------------
*  Converts the given fix32_t into a 32 byte signed integer.
*
*  value:			fix32_t value that should be converted.
*
*  returns:	Converted 32 byte signed integer
*
*/
INT32 fix32_to_int(fix32_t value)
{
#ifdef FIXMATH_NO_ROUNDING
	return (value >> 32);
#else
	if (value >= 0)
		return (value + (fix32_one >> 1)) / fix32_one;
	return (value - (fix32_one >> 1)) / fix32_one;
#endif
}

/*
* Function:  fix32_from_float
* --------------------
*  Converts the given float into a fix32_t value.
*
*  value:			Pointer to 4 bytes that contain a float.
*
*  returns:	Converted fix32_t value
*
*/
fix32_t fix32_from_float(const void* value)
{
	INT64 fi = *(INT64*)value;
	// get the fraction
	fix32_t q3232 = (fi & ((1 << 23) - 1)) | 1 << 23;
	// get the exponent
	INT64 expon = ((fi >> 23) & ((1 << 8) - 1)) - 127;
	// get the shift
	INT32 shift = 32 + expon - 23;
	// get the sign
	INT64 sign = (fi >> 31) & 1;

	if (shift >= 32 || shift <= -32)
	{
		q3232 = 0;
	}
	else
	{
		if (shift < 0)
		{
			q3232 >>= -shift;
		}
		else
		{
			q3232 <<= shift;
		}
	}

	if (sign)
	{
		INT64 int_part = (INT32)((q3232 & 0xFFFFFFFF00000000) >> 32);
		int_part *= -1;

		UINT32 frac_part = (UINT32)(q3232 & 0x00000000FFFFFFFF);
		if (frac_part)
		{
			frac_part = 0xFFFFFFFF - frac_part;
			q3232 &= 0xFFFFFFFF00000000;
			q3232 ^= frac_part;
			int_part -= 1;
		}
		q3232 &= 0x00000000FFFFFFFF;
		q3232 ^= int_part << 32;
	}

	return q3232;
}

/*
* Function:  count_bits_pow2
* --------------------
*  x.
*
*  x:				x.
*
*  returns:	x
*
*/
static INT32 count_bits_pow2(UINT64 x)
{
	INT32 l = -33;
	for (UINT64 i = 0; i < 64; i++)
	{
		UINT64 test = 1ULL << i;
		if (x >= test)
		{
			l++;
		}
		else
		{
			break;
		}
	}
	return l;
}

/*
* Function:  float_from_fix32
* --------------------
*  Converts the given float into a fix32_t value.
*
*  value:			fix32_t value that should be converted to a 4 byte float buffer.
*
*  returns:	float stored in a 4 byte unsigned integer
*
*/
UINT32 float_from_fix32(fix32_t value)
{
	INT64 original_num = (INT64)value;
	UINT64 sign = 0;
	if (original_num < 0)
	{
		sign = 1;
	}

	// remove the signed bit if it's set
	INT64 unsigned_ver = original_num < 0 ? -original_num : original_num;

	// calculate mantissa
	int lz = clz(unsigned_ver);
	UINT64 y = unsigned_ver << (lz + 1);

	// 33 --> because we use 64-bit fixed point num, the middle of it is at 32,
	// and +1 for the sign bit. Then 8 is the exponent bits, which is 8 for IEEE754
	UINT64 mantissa = y >> (33 + 8);

	// get the exponent bias ( 127 in IEEE754 ) to get exponent
	UINT64 exp = count_bits_pow2(unsigned_ver) + 127;

	// construct the final IEEE754 float binary number
	// first add the last 23 bits (mantissa)
	UINT32 ret = mantissa;

	// add exponent
	ret |= (exp << 23);

	// add the sign if needed
	if (sign)
	{
		ret |= 0x80000000;
	}

	return ret;
}

/*
* Function:  fix32_add
* --------------------
*  Combines two fix32_t values together.
*
*  inArg0:			First fix32_t value.
*  inArg1:			Second fix32_t value.
*
*  returns:	Combined sum of the values as fix32_t
*
*/
fix32_t fix32_add(fix32_t inArg0, fix32_t inArg1)
{
	// Use unsigned integers because overflow with signed integers is
	// an undefined operation (http://www.airs.com/blog/archives/120).
	UINT64 _a = inArg0, _b = inArg1;
	UINT64 sum = _a + _b;

	// Overflow can only happen if sign of a == sign of b, and then
	// it causes sign of sum != sign of a.
	if (!((_a ^ _b) & 0x8000000000000000) && ((_a ^ sum) & 0x8000000000000000))
		return fix32_overflow;

	return sum;
}

/*
* Function:  fix32_sub
* --------------------
*  Substracts a fix32_t from another fix32_t.
*
*  inArg0:			fix32_t from where to substract from.
*  inArg1:			fix32_t to substract.
*
*  returns:	Result from substraction operation as fix32_t
*
*/
fix32_t fix32_sub(fix32_t inArg0, fix32_t inArg1)
{
	UINT64 _a = inArg0, _b = inArg1;
	UINT64 diff = _a - _b;

	// Overflow can only happen if sign of a != sign of b, and then
	// it causes sign of diff != sign of a.
	if (((_a ^ _b) & 0x8000000000000000) && ((_a ^ diff) & 0x8000000000000000))
		return fix32_overflow;

	return diff;
}

/*
* Function:  fix32_mul
* --------------------
*  Multiplies the two fix32_t values together.
*
*  inArg0:			fix32_t value to multiply.
*  inArg1:			fix32_t value to multiply with.
*
*  returns:	Result of the multiplication operation as fix32_t
*
*/
fix32_t fix32_mul(fix32_t inArg0, fix32_t inArg1)
{
	// Each argument is divided to 32-bit parts.
	//					AB
	//			*	 CD
	// -----------
	//					BD	32 * 32 -> 64 bit products
	//				 CB
	//				 AD
	//				AC
	//			 |----| 64 bit product
	INT64 A = (inArg0 >> 32), C = (inArg1 >> 32);
	UINT64 B = (inArg0 & 0xFFFFFFFF), D = (inArg1 & 0xFFFFFFFF);

	INT64 AC = A * C;
	INT64 AD_CB = A * D + C * B;
	UINT64 BD = B * D;

	INT64 product_hi = AC + (AD_CB >> 32);

	// Handle carry from lower 32 bits to upper part of result.
	UINT64 ad_cb_temp = AD_CB << 32;
	UINT64 product_lo = BD + ad_cb_temp;
	if (product_lo < BD)
		product_hi++;

	// The upper 17 bits should all be the same (the sign).
	if (product_hi >> 63 != product_hi >> 31)
		return fix32_overflow;

	// Subtracting 0x80000000 (= 0.5) and then using signed right shift
	// achieves proper rounding to result-1, except in the corner
	// case of negative numbers and lowest word = 0x80000000.
	// To handle that, we also have to subtract 1 for negative numbers.
	UINT64 product_lo_tmp = product_lo;
	product_lo -= 0x80000000;
	product_lo -= (UINT64)product_hi >> 63;
	if (product_lo > product_lo_tmp)
		product_hi--;

	// Discard the lowest 16 bits. Note that this is not exactly the same
	// as dividing by 0x10000. For example if product = -1, result will
	// also be -1 and not 0. This is compensated by adding +1 to the result
	// and compensating this in turn in the rounding above.
	fix32_t result = (product_hi << 32) | (product_lo >> 32);
	result += 1;
	return result;
}

/*
* Function:  fix32_div
* --------------------
*  Divides the given fix32_t value with a fix32_t value.
*
*  inArg0:			fix32_t value to divide.
*  inArg1:			fix32_t value to divide with.
*
*  returns:	Result of the division operation as fix32_t
*
*/
fix32_t fix32_div(fix32_t inArg0, fix32_t inArg1)
{
	// This uses a hardware 64/64 bit division multiple times, until we have
	// computed all the bits in (inArg0<<33)/inArg1. Usually this takes 1-3 iterations.

	if (inArg1 == 0)
		return fix32_minimum;

	UINT64 remainder = (inArg0 >= 0) ? inArg0 : (-inArg0);
	UINT64 divider = (inArg1 >= 0) ? inArg1 : (-inArg1);
	UINT64 quotient = 0;
	int bit_pos = 33;

	// Kick-start the division a bit.
	// This improves speed in the worst-case scenarios where N and D are large
	// It gets a lower estimate for the result by N/(D >> 33 + 1).
	if (divider & 0xFFF0000000000000)
	{
		UINT64 shifted_div = ((divider >> 33) + 1);
		quotient = remainder / shifted_div;
		remainder -= ((UINT64)quotient * divider) >> 17;
	}

	// If the divider is divisible by 2^n, take advantage of it.
	while (!(divider & 0xF) && bit_pos >= 4)
	{
		divider >>= 4;
		bit_pos -= 4;
	}

	while (remainder && bit_pos >= 0)
	{
		// Shift remainder as much as we can without overflowing
		int shift = clz(remainder);
		if (shift > bit_pos)
			shift = bit_pos;
		remainder <<= shift;
		bit_pos -= shift;

		UINT64 div = remainder / divider;
		remainder = remainder % divider;
		quotient += div << bit_pos;

#ifndef FIXMATH_NO_OVERFLOW
		if (div & ~(0xFFFFFFFFFFFFFFFF >> bit_pos))
			return fix32_overflow;
#endif

		remainder <<= 1;
		bit_pos--;
	}

	fix32_t result = quotient >> 1;

	// Figure out the sign of the result
	if ((inArg0 ^ inArg1) & 0x8000000000000000)
	{
#ifndef FIXMATH_NO_OVERFLOW
		if (result == fix32_minimum)
			return fix32_overflow;
#endif

		result = -result;
	}

	return result;
}

/*
* Function:  fix32_sqrt
* --------------------
*  Calculates the square root of the given fix32_t value.
*
*  inValue:			fix32_t value to calculate the square root of.
*
*  returns:	Calculated square root of the value as fix32_t
*
*/
fix32_t fix32_sqrt(fix32_t inValue)
{
	UINT8 neg = (inValue < 0);
	UINT64 num = (neg ? -inValue : inValue);
	UINT64 result = 0;
	UINT64 bit;
	UINT8 n;

	bit = (UINT64)1 << 62;

	while (bit > num)
		bit >>= 2;

	// The main part is executed twice, in order to avoid
	// > 64 bit values in computations.
	for (n = 0; n < 2; n++)
	{
		// First we get the top 24 bits of the answer.
		while (bit)
		{
			if (num >= result + bit)
			{
				num -= result + bit;
				result = (result >> 1) + bit;
			}
			else
			{
				result = (result >> 1);
			}
			bit >>= 2;
		}

		if (n == 0)
		{
			// Then process it again to get the lowest bits.
			if (num > 4294967295)
			{
				// The remainder 'num' is too large to be shifted left
				// by 32, so we have to add 1 to result manually and
				// adjust 'num' accordingly.
				// num = a - (result + 0.5)^2
				//	 = num + result^2 - (result + 0.5)^2
				//	 = num - result - 0.5
				num -= result;
				num = (num << 32) - 0x80000000;
				result = (result << 32) + 0x80000000;
			}
			else
			{
				num <<= 32;
				result <<= 32;
			}

			bit = 1 << 30;
		}
	}

	// Finally, if next bit would have been 1, round the result upwards.
	if (num > result)
	{
		result++;
	}

	return (neg ? -(fix32_t)result : (fix32_t)result);
}

/*
* Function:  fix32_sin
* --------------------
*  Calculates the sin of the given fix32_t value.
*
*  inAngle:			fix32_t value to calculate the sin of.
*
*  returns:	Calculated sin of the value as fix32_t
*
*/
fix32_t fix32_sin(fix32_t inAngle)
{
	fix32_t tempAngle = inAngle % (fix32_pi << 1);

	if (tempAngle > fix32_pi)
		tempAngle -= (fix32_pi << 1);
	else if (tempAngle < -fix32_pi)
		tempAngle += (fix32_pi << 1);

	fix32_t tempAngleSq = fix32_mul(tempAngle, tempAngle);

	fix32_t tempOut = tempAngle;
	tempAngle = fix32_mul(tempAngle, tempAngleSq);
	tempOut -= (tempAngle / 6);
	tempAngle = fix32_mul(tempAngle, tempAngleSq);
	tempOut += (tempAngle / 120);
	tempAngle = fix32_mul(tempAngle, tempAngleSq);
	tempOut -= (tempAngle / 5040);
	tempAngle = fix32_mul(tempAngle, tempAngleSq);
	tempOut += (tempAngle / 362880);
	tempAngle = fix32_mul(tempAngle, tempAngleSq);
	tempOut -= (tempAngle / 39916800);
	tempAngle = fix32_mul(tempAngle, tempAngleSq);
	tempOut -= (tempAngle / 6227020800);

	return tempOut;
}

/*
* Function:  fix32_cos
* --------------------
*  Calculates the cos of the given fix32_t value.
*
*  inAngle:			fix32_t value to calculate the sin of.
*
*  returns:	Calculated cos of the value as fix32_t
*
*/
fix32_t fix32_cos(fix32_t inAngle)
{
	return fix32_sin(inAngle + (fix32_pi >> 1));
}

/*
* Function:  fix32_atan2
* --------------------
*  Calculates the atan2 of the given fix32_t values.
*
*  inY:				fix32_t value.
*  inX:				fix32_t value.
*
*  returns:	Calculated atan2 of the fix32_t values as fix32_t
*
*/
fix32_t fix32_atan2(fix32_t inY, fix32_t inX)
{
	fix32_t abs_inY, mask, angle, r, r_3;

	/* Absolute inY */
	mask = (inY >> (sizeof(fix32_t) * 8 - 1));
	abs_inY = (inY + mask) ^ mask;

	if (inX >= 0)
	{
		r = fix32_div((inX - abs_inY), (inX + abs_inY));
		r_3 = fix32_mul(fix32_mul(r, r), r);
		angle = fix32_mul(0x0000000031238038, r_3) - fix32_mul(0x00000000F8EED205, r) + PI_DIV_4;
	}
	else
	{
		r = fix32_div((inX + abs_inY), (abs_inY - inX));
		r_3 = fix32_mul(fix32_mul(r, r), r);
		angle = fix32_mul(0x0000000031238038, r_3) - fix32_mul(0x00000000F8EED205, r) + THREE_PI_DIV_4;
	}
	if (inY < 0)
	{
		angle = -angle;
	}

	return angle;
}

/*
* Function:  fix32_atan
* --------------------
*  Calculates the atan value of the given fix32_t value.
*
*  x:				fix32_t value.
*
*  returns:	Calculated atan value as fix32_t
*
*/
fix32_t fix32_atan(fix32_t x)
{
	return fix32_atan2(x, fix32_one);
}

/*
* Function:  fix32_asin
* --------------------
*  Calculates the asin value of the given fix32_t value.
*
*  x:				fix32_t value.
*
*  returns:	Calculated asin value as fix32_t
*
*/
fix32_t fix32_asin(fix32_t x)
{
	if ((x > fix32_one)
		|| (x < -fix32_one))
		return 0;

	fix32_t out;
	out = (fix32_one - fix32_mul(x, x));
	out = fix32_div(x, fix32_sqrt(out));
	out = fix32_atan(out);
	return out;
}