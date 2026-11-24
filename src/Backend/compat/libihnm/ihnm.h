/*
 * Compatibility shim for libllsm2.
 *
 * libllsm2's layer0.c includes <libihnm/ihnm.h>, but no libihnm symbol is
 * used by the copy of libllsm2 vendored in lib/llsm. Keeping an empty header
 * here lets us build against that source tree without adding an unused
 * dependency.
 */

#ifndef LESSAMPLER_COMPAT_LIBIHNM_IHNM_H
#define LESSAMPLER_COMPAT_LIBIHNM_IHNM_H

#endif//LESSAMPLER_COMPAT_LIBIHNM_IHNM_H
