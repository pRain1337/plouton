/*++
*
* Header file for fixed point related functionalities
*
* For more general implementation, see https://github.com/jussihi/libfixmath64
*
--*/

#ifndef __plouton_floatlib_h__
#define __plouton_floatlib_h__

// our includes
#include "../hardware/serial.h"

// Structures

#ifndef INT32_MAX
#define INT32_MAX 0x7fffffffL
#endif
#ifndef INT32_MIN
#define INT32_MIN (-INT32_MAX - 1L)
#endif

typedef union {
	struct {
		UINT32 fraction : 23;
		UINT32 exp_bias : 8;
		UINT32 sign : 1;
	} fields;

	INT32 dword;
	INT32 float_value;
} IEEE745_float;

typedef INT64 fix32_t;

// Definitions

static const fix32_t FOUR_DIV_PI = 0x0000000145F306DD;			/*!< Fix32 value of 4/PI */
static const fix32_t _FOUR_DIV_PI2 = 0xFFFFFFFF983f4277;		/*!< Fix32 value of -4/PI² */
static const fix32_t X4_CORRECTION_COMPONENT = 0x3999999A;      /*!< Fix32 value of 0.225 */
static const fix32_t PI_DIV_4 = 0x00000000C90FDAA2;				/*!< Fix32 value of PI/4 */
static const fix32_t THREE_PI_DIV_4 = 0x000000025B2F8FE6;       /*!< Fix32 value of 3PI/4 */

static const fix32_t fix32_maximum = 0x7FFFFFFFFFFFFFFF;		/*!< the maximum value of fix32_t */
static const fix32_t fix32_minimum = 0x8000000000000000;		/*!< the maximum value of fix32_t */
static const fix32_t fix32_overflow = 0x8000000000000000;		/*!< the value used to indicate overflows when FIXMATH_NO_OVERFLOW is not specified */

static const fix32_t fix32_pi = 0x00000003243F6A88;				/*!< fix32_t value of pi */
static const fix32_t fix32_e = 0x00000002B7E15163;				/*!< fix32_t value of e */

static const fix32_t fix32_fivethousand = 0x138800000000;		/*!< fix32_t value of 5000 */
static const fix32_t fix32_1920 = 0x0000078000000000;			/*!< fix32_t value of 1920 */
static const fix32_t fix32_1080 = 0x0000043800000000;			/*!< fix32_t value of 1080 */
static const fix32_t fix32_thousand = 0x000003e800000000;       /*!< fix32_t value of 1000 */
static const fix32_t fix32_360 = 0x0000016800000000;			/*!< fix32_t value of 360 */
static const fix32_t fix32_270 = 0x0000010e00000000;			/*!< fix32_t value of 270 */
static const fix32_t fix32_180 = 0x000000b400000000;			/*!< fix32_t value of 180 */
static const fix32_t fix32_100 = 0x0000006400000000;			/*!< fix32_t value of 100 */
static const fix32_t fix32_90 = 0x0000005a00000000;				/*!< fix32_t value of 90 */
static const fix32_t fix32_89 = 0x0000005900000000;				/*!< fix32_t value of 89 */
static const fix32_t fix32_70 = 0x0000004600000000;				/*!< fix32_t value of 70 */
static const fix32_t fix32_50 = 0x0000003200000000;				/*!< fix32_t value of 50 */
static const fix32_t fix32_25 = 0x0000001900000000;				/*!< fix32_t value of 25 */
static const fix32_t fix32_21 = 0x0000001500000000;				/*!< fix32_t value of 21 */
static const fix32_t fix32_20 = 0x0000001400000000;				/*!< fix32_t value of 20 */
static const fix32_t fix32_17 = 0x0000001100000000;				/*!< fix32_t value of 17 */
static const fix32_t fix32_16 = 0x0000001000000000;				/*!< fix32_t value of 16 */
static const fix32_t fix32_15 = 0x0000000f00000000;				/*!< fix32_t value of 15 */
static const fix32_t fix32_13 = 0x0000000d00000000;				/*!< fix32_t value of 13 */
static const fix32_t fix32_12 = 0x0000000c00000000;				/*!< fix32_t value of 12 */
static const fix32_t fix32_10 = 0x0000000a00000000;				/*!< fix32_t value of 10 */
static const fix32_t fix32_8 = 0x0000000800000000;				/*!< fix32_t value of 8 */
static const fix32_t fix32_7 = 0x0000000600000000;				/*!< fix32_t value of 7 */
static const fix32_t fix32_6 = 0x0000000600000000;				/*!< fix32_t value of 6 */
static const fix32_t fix32_5 = 0x0000000500000000;				/*!< fix32_t value of 5 */
static const fix32_t fix32_4 = 0x0000000400000000;				/*!< fix32_t value of 4 */
static const fix32_t fix32_two = 0x0000000200000000;			/*!< fix32_t value of 2 */
static const fix32_t fix32_one = 0x0000000100000000;			/*!< fix32_t value of 1 */
static const fix32_t fix32_zero = 0x0000000000000000;			/*!< fix32_t value of 0 */
static const fix32_t fix32_05 = 0x0000000080000000;				/*!< fix32_t value of 0.5f */
static const fix32_t fix32_022 = 0x0000000005A1CAC0;			/*!< fix32_t value of 0.022f */
static const fix32_t fix32_0015625 = 0x0000000004000000;		/*!< fix32_t value of 0.015625f */
static const fix32_t fix32_neg_one = 0xffffffff00000000;		/*!< fix32_t value of -1 */
static const fix32_t fix32_neg_89 = 0xffffffa700000000;			/*!< fix32_t value of -89 */
static const fix32_t fix32_neg_180 = 0xffffff4c00000000;		/*!< fix32_t value of -180 */
static const fix32_t fix32_neg_360 = 0xfffffe9800000000;		/*!< fix32_t value of -360 */

static const int IEEE745_FLOAT_BIAS = 127;

// Functions

/*
* Conversion functions between fix16_t and integer.
* These are inlined to allow compiler to optimize away constant numbers
*/

static inline fix32_t fix32_from_int(INT32 a)
{ 
	return a * fix32_one; 
}

static inline INT64 int_from_fix32(fix32_t a)
{ 
	return a / fix32_one; 
}

/* Macro for defining fix16_t constant values.
   The functions above can't be used from e.g. global variable initializers,
   and their names are quite long also. This macro is useful for constants
   springled alongside code, e.g. F16(1.234).
   Note that the argument is evaluated multiple times, and also otherwise
   you should only use this for constant values. For runtime-conversions,
   use the functions above.
*/
#define F16(x) ((fix16_t)(((x) >= 0) ? ((x) * 65536.0 + 0.5) : ((x) * 65536.0 - 0.5)))

static inline fix32_t fix32_abs(fix32_t x)
{ 
	return (x < 0 ? -x : x); 
}

static inline fix32_t fix32_floor(fix32_t x)
{ 
	return (x & 0xFFFFFFFF00000000ULL); 
}

static inline fix32_t fix32_ceil(fix32_t x)
{ 
	return (x & 0xFFFFFFFF00000000ULL) + (x & 0x00000000FFFFFFFFULL ? fix32_one : 0); 
}

static inline fix32_t fix32_min(fix32_t x, fix32_t y)
{ 
	return (x < y ? x : y); 
}

static inline fix32_t fix32_max(fix32_t x, fix32_t y)
{ 
	return (x > y ? x : y); 
}

static inline fix32_t fix32_clamp(fix32_t x, fix32_t lo, fix32_t hi)
{ 
	return fix32_min(fix32_max(x, lo), hi);
}

/*
 * Comparator function, returns a positive if a is larger, 
 * negative if b is larger.
 */
static inline BOOLEAN fix32_cmp(fix32_t a, fix32_t b)
{ 
	return a == b; 
}

static inline BOOLEAN fix32_larger(fix32_t a, fix32_t b)
{ 
	return a > b; 
}
    
static inline BOOLEAN fix32_largereq(fix32_t a, fix32_t b)
{ 
	return a >= b; 
}
   
static inline BOOLEAN fix32_less(fix32_t a, fix32_t b)
{
	return a < b; 
}
    
static inline BOOLEAN fix32_lesseq(fix32_t a, fix32_t b)
{ 
	return a <= b; 
}
   
/*
* Function:  fix32_sqrt
* --------------------
* Converts a float to a an integer with no decimals.
*/
INT32 fix32_to_int(fix32_t value);

/*
* Function:  float2int
* --------------------
* xxx.
*/
INT32 float2int(const void* value);

/*
* Function:  fix32_from_float
* --------------------
* Converts a float to a fixed point q31.32 integer 
*/
fix32_t fix32_from_float(const void* value);

/*
* Function:  float_from_fix32
* --------------------
* Converts a q31.32 integer back to IEEE754 floating point.
*/
UINT32 float_from_fix32(fix32_t value);

/*
* Function:  fix32_add
* --------------------
* Adds the two given fix32_t together and returns the results.
*/
fix32_t fix32_add(fix32_t a, fix32_t b);

/*
* Function:  fix32_sub
* --------------------
* Substracts the second fix32_t from the first fix_32 and returns the result.
*/
fix32_t fix32_sub(fix32_t a, fix32_t b);

/*
* Function:  fix32_mul
* --------------------
* Multiplies the two given fix32_t's and returns the result.
*/
fix32_t fix32_mul(fix32_t inArg0, fix32_t inArg1);

/*
* Function:  fix32_div
* --------------------
* Divides the first given fix32_t by the second and returns the result.
*/
fix32_t fix32_div(fix32_t a, fix32_t b);

/*
* Function:  fix32_sqrt
* --------------------
* Returns the square root of the given fix32_t.
*/
fix32_t fix32_sqrt(fix32_t inValue);

/*
* Function:  fix32_sin
* --------------------
* Returns the sine of the given fix32_t using a lookup table.
*/
fix32_t fix32_sin(fix32_t inAngle);

/*
* Function:  fix32_cos
* --------------------
* Returns the cosine of the given fix32_t using fix32_sin.
*/
fix32_t fix32_cos(fix32_t inAngle);

/*
* Function:  fix32_atan2
* --------------------
* Returns the arctangent of inY/inX..
*/
fix32_t fix32_atan2(fix32_t inY , fix32_t inX);

#endif