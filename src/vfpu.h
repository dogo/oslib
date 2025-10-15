/**
 * @file vfpu.h
 * @brief VFPU functions for PSP.
 *
 * This file contains functions that utilize the PSP's Vector Floating Point Unit (VFPU)
 * for performing fast mathematical operations. These functions are particularly optimized
 * for angles given in radians and operations involving a radius (rayon).
 */

#ifndef __OSL_VFPU_H__
#define __OSL_VFPU_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup vfpu VFPU functions
 * @brief Functions using the Vector Floating Point Unit (VFPU).
 *
 * This module provides functions that perform various mathematical operations using
 * the VFPU on the PSP. These operations are optimized for speed and efficiency.
 * @{
 */

#ifdef PSP

#include <psptypes.h>
#include "pspmath.h"

#endif

/**
 * @brief Calculates the sine of an angle using the VFPU.
 *
 * This function computes the sine of an angle (given in radians) multiplied by the radius (rayon).
 * It is optimized for very fast execution on the PSP's VFPU.
 *
 * @param angle The angle in radians.
 * @param rayon The radius (multiplier).
 * @return The sine of the angle multiplied by the radius.
 *
 * @note This function uses the VFPU for fast computation. For angles in degrees, consider using #oslSin.
 */
extern float oslVfpu_sinf(float angle, float rayon);

/**
 * @brief Calculates the cosine of an angle using the VFPU.
 *
 * This function computes the cosine of an angle (given in radians) multiplied by the radius (rayon).
 * It is optimized for very fast execution on the PSP's VFPU.
 *
 * @param angle The angle in radians.
 * @param rayon The radius (multiplier).
 * @return The cosine of the angle multiplied by the radius.
 *
 * @note This function uses the VFPU for fast computation. For angles in degrees, consider using #oslCos.
 */
extern float oslVfpu_cosf(float angle, float rayon);

/** @} */ // end of vfpu

#ifdef __cplusplus
}
#endif

#endif // __OSL_VFPU_H__
