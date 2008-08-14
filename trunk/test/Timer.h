////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2008, Massachusetts Institute of Technology              //
// All rights reserved.                                                   //
//                                                                        //
// Redistribution and use in source and binary forms, with or without     //
// modification, are permitted provided that the following conditions are //
// met:                                                                   //
//                                                                        //
//     * Redistributions of source code must retain the above copyright   //
//       notice, this list of conditions and the following disclaimer.    //
//                                                                        //
//     * Redistributions in binary form must reproduce the above          //
//       copyright notice, this list of conditions and the following      //
//       disclaimer in the documentation and/or other materials provided  //
//       with the distribution.                                           //
//                                                                        //
//     * Neither the name of the Massachusetts Institute of Technology    //
//       nor the names of its contributors may be used to endorse or      //
//       promote products derived from this software without specific     //
//       prior written permission.                                        //
//                                                                        //
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS    //
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT      //
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR  //
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT   //
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,  //
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT       //
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,  //
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY  //
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT    //
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  //
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.   //
////////////////////////////////////////////////////////////////////////////

#ifndef TIMER_H
#define TIMER_H

extern "C" {
#if defined(__i386__)
    static inline uint64_t rdtsc(void)
    {
        uint64_t x;
        __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
        return x;
    }
#elif defined(__x86_64__)
    static inline uint64_t rdtsc() {
        uint32_t lo, hi;
        __asm__ __volatile__ (
            "xorl %%eax, %%eax\n"
            "cpuid\n"
            "rdtsc\n"
            : "=a" (lo), "=d" (hi) : : "%ebx", "%ecx");
        return (uint64_t)hi << 32 | lo;
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////

#define __NUMITER 3
#define __MINCYCLES 100000000

#define START_TIME \
    __bestTime = 1e100; \
    for (size_t i = 0; i < __NUMITER; ++i) { \
        __totN = 0;  __startCycle = rdtsc(); \
        do { \
            __totN++;
#define START_TIME_N(N) \
    __bestTime = 1e100; \
    for (size_t i = 0; i < __NUMITER; ++i) { \
        __totN = 0;  __startCycle = rdtsc(); \
        do { \
            __totN += N;
#define END_TIME(var) \
        } while (rdtsc() - __startCycle < __MINCYCLES); \
        __bestTime = std::min(__bestTime, \
                              (float)(rdtsc()-__startCycle) / __totN); \
    } \
    float var = __bestTime;

uint64_t __startCycle, __minCycle;
size_t   __totN;
float    __bestTime;


#endif // TIMER_H
