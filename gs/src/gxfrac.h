/* Portions Copyright (C) 2001 artofcode LLC.
   Portions Copyright (C) 1996, 2001 Artifex Software Inc.
   Portions Copyright (C) 1988, 2000 Aladdin Enterprises.
   This software is based in part on the work of the Independent JPEG Group.
   All Rights Reserved.

   This software is distributed under license and may not be copied, modified
   or distributed except as expressly authorized under the terms of that
   license.  Refer to licensing information at http://www.artifex.com/ or
   contact Artifex Software, Inc., 101 Lucas Valley Road #110,
   San Rafael, CA  94903, (415)492-9861, for further information. */

/*$RCSfile$ $Revision$ */
/* Fraction representation for Ghostscript */

#ifndef gxfrac_INCLUDED
#  define gxfrac_INCLUDED

/*
 * Represent a fraction in [0.0..1.0].  Note that the 1.0 endpoint is
 * included.  Since undercolor removal requires a signed frac, we limit
 * fracs to 15 bits rather than 16.
 */
typedef short frac;
typedef short signed_frac;

#define arch_log2_sizeof_frac arch_log2_sizeof_short
#define arch_sizeof_frac arch_sizeof_short
#define FRAC_BITS 15
#define frac_0 ((frac)0)

/*
 * Normally one would represent a fractional value of this kind as a short
 * integer, in [-32767..32767].  Unfortunately, this approach cannot
 * represent any of the common values like 1/2, 1/3, or 1/5 exactly, causing
 * rounding errors.  Instead, we opt for using the range [-32760..32760],
 * which allows exact representation of almost all commonly used fractions
 * (e.g., N/360 for 0<=N<=360).
 */
#define frac_1_0bits 3
#define frac_1 ((frac)0x7ff8)
#define frac_1_long ((long)frac_1)
#define frac_1_float ((float)frac_1)
/* Conversion between fracs and floats. */
#define frac2float(fr) ((fr) / frac_1_float)
#define float2frac(fl) ((frac)(((fl) + 0.5 / frac_1_float) * frac_1_float))

/*
 * Conversion between unsigned fracs and bytes (or, in general,
 * shorter integers) representing fractions. This is highly dependent
 * on the definition of frac_1 above.
 */
#define _frac2s(fr)\
  (((fr) >> (FRAC_BITS - frac_1_0bits)) + (fr))
#define frac2bits(fr, nb)\
  ((uint)(_frac2s(fr) >> (FRAC_BITS - (nb))))
#define frac2byte(fr) ((byte)frac2bits(fr, 8))
/* bits2frac requires frac_bits / 2 <= nb <= frac_bits. */
#define bits2frac(v, nb) ((frac)(\
  ((frac)(v) << (FRAC_BITS - (nb))) +\
   ((v) >> ((nb) * 2 - FRAC_BITS)) -\
   ((v) >> ((nb) - frac_1_0bits)) ))
#define byte2frac(b) bits2frac(b, 8)
/* Produce a result that is guaranteed to convert back to a frac */
/* not exceeding the original value fr. */
#define frac2bits_floor(fr, nb)\
  ((uint)((_frac2s(fr) - (_frac2s(fr) >> (nb))) >> (FRAC_BITS - (nb))))
/*
 * Conversion between fracs and unsigned shorts.
 */
#define ushort_bits (arch_sizeof_short * 8)
#define frac2ushort(fr) ((ushort)(\
  ((fr) << (ushort_bits - FRAC_BITS)) +\
  ((fr) >> (FRAC_BITS * 2 - ushort_bits - frac_1_0bits)) ))
#define ushort2frac(us) ((frac)(\
  ((us) >> (ushort_bits - FRAC_BITS)) -\
  ((us) >> (ushort_bits - frac_1_0bits)) ))
/*
 * Compute the quotient Q = floor(P / frac_1),
 * where P is the (ulong) product of a uint or ushort V and a frac F.
 * See gxarith.h for the underlying algorithm.
 */
#define frac_1_quo(p)\
  ( (((p) >> frac_1_0bits) + ((p) >> FRAC_BITS) + 1) >> (FRAC_BITS - frac_1_0bits) )
/*
 * Compute the remainder similarly, having already computed the quotient.
 * This is, of course, P - Q * frac_1.
 */
#define frac_1_rem(p, q)\
  ((frac)( (uint)(p) - ((q) << FRAC_BITS) + ((q) << frac_1_0bits) ))

#endif /* gxfrac_INCLUDED */
