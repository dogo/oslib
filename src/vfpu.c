#include "oslib.h"
#include "vfpu_ops.h"

#ifdef PSP

// VFPU variables aligned to 64 bytes
float vfpu_vars[4] __attribute__((aligned(64)));

// Trigonometric constants
#define DEG_TO_RAD 0.0174532925f

// Avoid using these external methods as they are deprecated
// r = f1 + f2
extern float vfpu_add(float f1, float f2);
// r = f1 - f2
extern float vfpu_sub(float f1, float f2);
// r = f1 * f2
extern float vfpu_mul(float f1, float f2);
// r = f1 / f2
extern float vfpu_div(float f1, float f2);
// r = (int)v
extern int vfpu_f2i(float v);
// r = (float)v
extern float vfpu_i2f(int v);
// r = sin(f1°) * f2
extern float vfpu_sini(int f1, int f2);
// r = cos(f1°) * f2
extern float vfpu_cosi(int f1, int f2);
// r = (int)(f1 - f2)
extern int vfpu_isubf(float f1, float f2);

// VFPU Assembly Macros
#define vi2f_s(vfpu_rd,vfpu_rs,scale) (0xd2800000 | ((scale) << 16) | ((vfpu_rs) << 8) | (vfpu_rd))
#define vf2in_s(vfpu_rd,vfpu_rs,scale) (0xd2000000 | ((scale) << 16) | ((vfpu_rs) << 8) | (vfpu_rd))

// Wrapper function for sine using VFPU
float oslVfpu_sinf(float f1, float f2) {
   register void *ptr __asm ("a0") = vfpu_vars;
   *(float*)ptr = f1;
   *((float*)ptr + 1) = f2;
   __asm__ volatile (
      cgen_asm(lv_s(0, 0, R_a0, 0))              // Load f1
      cgen_asm(lv_s(1, 1, R_a0, 0))              // Load f2

      // Trigonometric functions expect input as angle*2/PI
      cgen_asm(vcst_s(2, 5))                     // Set constant 2/PI
      cgen_asm(vmul_s(0, 0, 2))                  // Multiply angle by 2/PI

      cgen_asm(vsin_s(124, 0))                   // Compute sin(f1)
      cgen_asm(vmul_s(124, 124, 1))              // Multiply sin(f1) by f2
      cgen_asm(sv_q(31, 0 * 4, R_a0, 0))         // Store result
   : "=r"(ptr) : "r"(ptr) : "memory");

   return vfpu_vars[0];
}

// Wrapper function for cosine using VFPU
float oslVfpu_cosf(float f1, float f2) {
   register void *ptr __asm ("a0") = vfpu_vars;
   *(float*)ptr = f1;
   *((float*)ptr + 1) = f2;
   __asm__ volatile (
      cgen_asm(lv_s(0, 0, R_a0, 0))              // Load f1
      cgen_asm(lv_s(1, 1, R_a0, 0))              // Load f2

      // Trigonometric functions expect input as angle*2/PI
      cgen_asm(vcst_s(2, 5))                     // Set constant 2/PI
      cgen_asm(vmul_s(0, 0, 2))                  // Multiply angle by 2/PI

      cgen_asm(vcos_s(124, 0))                   // Compute cos(f1)
      cgen_asm(vmul_s(124, 124, 1))              // Multiply cos(f1) by f2
      cgen_asm(sv_q(31, 0 * 4, R_a0, 0))         // Store result
   : "=r"(ptr) : "r"(ptr) : "memory");

   return vfpu_vars[0];
}

// Cosine function wrapper with degree-to-radian conversion
float oslCos(float angle, float dist) {
   register void *ptr __asm ("a0") = vfpu_vars;
   *(float*)ptr = angle;
   *((float*)ptr + 1) = dist;
   *((float*)ptr + 2) = 90.0f;

   __asm__ volatile (
      cgen_asm(lv_s(0, 0, R_a0, 0))              // Load angle
      cgen_asm(lv_s(1, 1, R_a0, 0))              // Load distance
      cgen_asm(lv_s(2, 2, R_a0, 0))              // Load 90.0

      cgen_asm(vdiv_s(0, 0, 2))                  // Divide angle by 90

      cgen_asm(vcos_s(124, 0))                   // Compute cos(angle)
      cgen_asm(vmul_s(124, 124, 1))              // Multiply by distance

      cgen_asm(sv_q(31, 0 * 4, R_a0, 0))         // Store result
   : "=r"(ptr) : "r"(ptr) : "memory");

   return vfpu_vars[0];
}

// Sine function wrapper with degree-to-radian conversion
float oslSin(float angle, float dist) {
   register void *ptr __asm ("a0") = vfpu_vars;
   *(float*)ptr = angle;
   *((float*)ptr + 1) = dist;
   *((float*)ptr + 2) = 90.0f;

   __asm__ volatile (
      cgen_asm(lv_s(0, 0, R_a0, 0))              // Load angle
      cgen_asm(lv_s(1, 1, R_a0, 0))              // Load distance
      cgen_asm(lv_s(2, 2, R_a0, 0))              // Load 90.0

      cgen_asm(vdiv_s(0, 0, 2))                  // Divide angle by 90

      cgen_asm(vsin_s(124, 0))                   // Compute sin(angle)
      cgen_asm(vmul_s(124, 124, 1))              // Multiply by distance

      cgen_asm(sv_q(31, 0 * 4, R_a0, 0))         // Store result
   : "=r"(ptr) : "r"(ptr) : "memory");

   return vfpu_vars[0];
}

#else // Non-PSP implementations using standard math library

#include <math.h>

float vfpu_sini(int f1, int f2) {
    return sinf(f1 * DEG_TO_RAD) * f2;
}

float vfpu_cosi(int f1, int f2) {
    return cosf(f1 * DEG_TO_RAD) * f2;
}

float vfpu_cosf(float f1, float f2) {
    return cosf(f1) * f2;
}

float vfpu_sinf(float f1, float f2) {
    return sinf(f1) * f2;
}

// Cosine function wrapper for non-PSP
float oslCos(float angle, float dist) {
    return vfpu_cosf(angle * DEG_TO_RAD, dist);
}

// Sine function wrapper for non-PSP
float oslSin(float angle, float dist) {
    return vfpu_sinf(angle * DEG_TO_RAD, dist);
}

#endif
