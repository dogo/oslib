#ifndef __OSL_MATH_H__
#define __OSL_MATH_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup math Math
 *
 * Mathematical functions utilizing the PSP's Vector Floating Point Unit (VFPU).
 *
 * These functions provide efficient mathematical computations by leveraging the
 * VFPU on the PSP. They are based on the `libpspmath` library (version 4) by MrMr[iCE].
 * @{
 */

#include "pspmath.h"

/**
 * @brief Set the seed for the VFPU random number generator.
 *
 * This function sets the initial seed value for generating random numbers using
 * the VFPU random number generator.
 *
 * @param x The seed value to initialize the random number generator.
 */
#define oslSrand vfpu_srand

/**
 * @brief Generate a random floating-point value.
 *
 * This function returns a random floating-point number between the specified minimum
 * and maximum values using the VFPU.
 *
 * @param min The minimum value that can be returned.
 * @param max The maximum value that can be returned.
 * @return A random floating-point number between `min` and `max`.
 */
#define oslRandf vfpu_randf

/**
 * @brief Generate a random color value in 8888 format.
 *
 * This function returns a random color value in the 8888 format (8 bits per channel).
 * The alpha channel is always set to 0xFF.
 *
 * @param min The minimum value for each color channel (range: 0 to 255).
 * @param max The maximum value for each color channel (range: 0 to 255).
 * @return A random color value in 8888 format.
 */
#define oslRand_8888 vfpu_rand_8888

/**
 * @brief Calculate the sine of an angle.
 *
 * This function returns the sine of the given angle (in radians) using the VFPU.
 *
 * @param x The angle in radians.
 * @return The sine of the input angle.
 */
#define oslSinf vfpu_sinf

/**
 * @brief Calculate the cosine of an angle.
 *
 * This function returns the cosine of the given angle (in radians) using the VFPU.
 *
 * @param x The angle in radians.
 * @return The cosine of the input angle.
 */
#define oslCosf vfpu_cosf

/**
 * @brief Calculate the tangent of an angle.
 *
 * This function returns the tangent of the given angle (in radians) using the VFPU.
 *
 * @param x The angle in radians.
 * @return The tangent of the input angle.
 */
#define oslTanf vfpu_tanf

/**
 * @brief Calculate the inverse sine (arcsin) of a value.
 *
 * This function returns the inverse sine (arcsin) of the input value using the VFPU.
 *
 * @param x The input value.
 * @return The arcsin of the input value.
 */
#define oslAsinf vfpu_asinf

/**
 * @brief Calculate the inverse cosine (arccos) of a value.
 *
 * This function returns the inverse cosine (arccos) of the input value using the VFPU.
 *
 * @param x The input value.
 * @return The arccos of the input value.
 */
#define oslAcosf vfpu_acosf

/**
 * @brief Calculate the inverse tangent (arctan) of a value.
 *
 * This function returns the inverse tangent (arctan) of the input value using the VFPU.
 *
 * @param x The input value.
 * @return The arctan of the input value.
 */
#define oslAtanf vfpu_atanf

/**
 * @brief Calculate the inverse tangent of two values, with quadrant correction.
 *
 * This function returns the inverse tangent (arctan) of `y/x` using the VFPU,
 * with correct quadrant determination.
 *
 * @param y The y-coordinate.
 * @param x The x-coordinate.
 * @return The arctan of the inputs, with quadrant correction.
 */
#define oslAtan2f vfpu_atan2f

/**
 * @brief Calculate the hyperbolic sine of a value.
 *
 * This function returns the hyperbolic sine (sinh) of the input value using the VFPU.
 *
 * @param x The input value.
 * @return The hyperbolic sine of the input value.
 */
#define oslSinhf vfpu_sinhf

/**
 * @brief Calculate the hyperbolic cosine of a value.
 *
 * This function returns the hyperbolic cosine (cosh) of the input value using the VFPU.
 *
 * @param x The input value.
 * @return The hyperbolic cosine of the input value.
 */
#define oslCoshf vfpu_coshf

/**
 * @brief Calculate the hyperbolic tangent of a value.
 *
 * This function returns the hyperbolic tangent (tanh) of the input value using the VFPU.
 *
 * @param x The input value.
 * @return The hyperbolic tangent of the input value.
 */
#define oslTanhf vfpu_tanhf

/**
 * @brief Calculate both sine and cosine of an angle.
 *
 * This function computes both the sine and cosine of the given angle (in radians)
 * and stores the results in the provided pointers.
 *
 * @param r The angle in radians.
 * @param s A pointer to store the sine of the angle.
 * @param c A pointer to store the cosine of the angle.
 */
#define oslSincos vfpu_sincos

/**
 * @brief Calculate the exponential of a value.
 *
 * This function returns the exponential of the input value (e^x) using the VFPU.
 *
 * @param x The input value.
 * @return The exponential of the input value.
 */
#define oslExpf vfpu_expf

/**
 * @brief Calculate the natural logarithm of a value.
 *
 * This function returns the natural logarithm (log base e) of the input value using the VFPU.
 *
 * @param x The input value.
 * @return The natural logarithm of the input value.
 */
#define oslLogf vfpu_logf

/**
 * @brief Calculate the power of a number.
 *
 * This function returns the value of `x` raised to the power of `y` using the VFPU.
 *
 * @param x The base value.
 * @param y The exponent value.
 * @return The result of `x` raised to the power of `y`.
 */
#define oslPowf vfpu_powf

/**
 * @brief Calculate the floating-point remainder of division.
 *
 * This function returns the floating-point remainder of `x/y` using the VFPU.
 *
 * @param x The numerator.
 * @param y The denominator.
 * @return The floating-point remainder of `x/y`.
 */
#define oslFmodf vfpu_fmodf

/** @} */ // end of oslMath

#ifdef __cplusplus
}
#endif

#endif // __OSL_MATH_H__
