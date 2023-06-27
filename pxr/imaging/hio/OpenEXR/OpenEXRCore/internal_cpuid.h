/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/
#ifndef IMF_INTERNAL_CPUID_H_HAS_BEEN_INCLUDED
#define IMF_INTERNAL_CPUID_H_HAS_BEEN_INCLUDED

#if defined(__x86_64__) || defined(_M_X64)
#    if defined(_MSC_VER) && defined(_WIN32)
#        include <intrin.h>
#    else
#        include <cpuid.h>
#    endif
#endif

static inline void
check_for_x86_simd (int* f16c, int* avx, int* sse2)
{
// Only use compiler flags on e2k (MCST Elbrus 2000) architecture
#ifdef __e2k__
#    if defined(__SSE2__)
    *sse2 = 1;
#    endif
#    if defined(__AVX__)
    *avx = 1;
#    endif
#    if defined(__F16C__)
    *f16c = 1;
#    endif

#else
#    if (defined(__x86_64__) || defined(_M_X64))

#        if defined(__AVX__) && defined(__F16C__)
    *f16c = 1;
    *avx  = 1;
    *sse2 = 1;
#        else
#            ifdef _WIN32
    int regs[4], osxsave;

    __cpuid (regs, 0);
    if (regs[0] >= 1) { __cpuidex (regs, 1, 0); }
    else
        regs[2] = 0;
#            else
    unsigned int regs[4], osxsave;
    __get_cpuid (0, &regs[0], &regs[1], &regs[2], &regs[3]);
    if (regs[0] >= 1)
    {
        __get_cpuid (1, &regs[0], &regs[1], &regs[2], &regs[3]);
    }
    else
        regs[2] = 0;
#            endif
    /* AVX is indicated by bit 28, F16C by 29 of ECX (reg 2) */
    osxsave = (regs[2] & (1 << 27)) ? 1 : 0;
    *avx    = (regs[2] & (1 << 28)) ? 1 : 0;
    *f16c   = (regs[2] & (1 << 29)) ? 1 : 0;
    /* sse2 is in EDX bit 26 */
    *sse2 = (regs[3] & (1 << 26)) ? 1 : 0;

    if (!osxsave)
    {
        *avx  = 0;
        *f16c = 0;
    }
    else
    {
        /* check extended control register */
#            if defined(OPENEXR_IMF_HAVE_GCC_INLINE_ASM_AVX) &&                \
                (defined(_M_X64) || defined(__x86_64__))
        __asm__ __volatile__ ("xgetbv"
                              : /* Output  */ "=a"(regs[0]), "=d"(regs[3])
                              : /* Input   */ "c"(0)
                              : /* Clobber */);
        /* eax bit 1 - SSE managed, bit 2 - AVX managed */
        if ((regs[0] & 6) != 6)
        {
            *avx  = 0;
            *f16c = 0;
        }
#            else
        *avx    = 0;
        *f16c   = 0;
#            endif
    }
#        endif
#    else
    *f16c = 0;
    *avx  = 0;
    *sse2 = 0;
#    endif
#endif
}

static inline int
has_native_half (void)
{
#if defined(__x86_64__) || defined(_M_X64)
    int sse2, avx, f16c;
    check_for_x86_simd (&f16c, &avx, &sse2);
    return avx && f16c;
#else
    // TODO: add case for neon?
    return 0;
#endif
}

#endif
