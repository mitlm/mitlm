////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2008, Massachusetts Institute of Technology              //
// Copyright (c) 2013, Giulio Paci <giuliopaci@gmail.com>                 //
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

#ifndef BITOPS_H
#define BITOPS_H

namespace mitlm {

///////////////////////////////////////////////////////////////////////////////

// Find the last (most-significant) bit set.
// Note fls(0) = 0, fls(1) = 1, fls(0x80000000) = 32.
static __inline__ unsigned long find_last_bit_set(unsigned long x) {
#ifdef HAVE_X86_ASM
    #if __WORDSIZE == 64
        if (!x) return 0;
        __asm__("bsrq %1,%0"
                :"=r" (x)
                :"rm" (x));
        return x + 1;
    #else
        int r;
        __asm__("bsrl %1,%0\n"
                "jnz 1f\n"
                "movl $-1,%0\n"
                "1:"
                :"=r" (r)
                :"rm" (x));
        return r + 1;
    #endif
#else
#ifdef HAVE___BUILTIN_CLZ
return (x? (((sizeof(unsigned long)*8))- __builtin_clzl(x)): 0);
#else
    #if __WORDSIZE == 64
	int r = 64;
	if (!x)  return 0;
	if (!(x & 0xffffffff00000000ul)) { x <<= 32; r -= 32; }
	if (!(x & 0xffff000000000000ul)) { x <<= 16; r -= 16; }
	if (!(x & 0xff00000000000000ul)) { x <<= 8;  r -= 8;  }
	if (!(x & 0xf000000000000000ul)) { x <<= 4;  r -= 4;  }
	if (!(x & 0xc000000000000000ul)) { x <<= 2;  r -= 2;  }
	if (!(x & 0x8000000000000000ul)) { x <<= 1;  r -= 1;  }
	return r;
    #else
	int r = 32;
	if (!x)  return 0;
	if (!(x & 0xffff0000ul)) { x <<= 16; r -= 16; }
	if (!(x & 0xff000000ul)) { x <<= 8;  r -= 8;  }
	if (!(x & 0xf0000000ul)) { x <<= 4;  r -= 4;  }
	if (!(x & 0xc0000000ul)) { x <<= 2;  r -= 2;  }
	if (!(x & 0x80000000ul)) { x <<= 1;  r -= 1;  }
	return r;
    #endif
#endif
#endif
}

// Determine if x is a power of 2 or 0.
static __inline__ bool isPowerOf2(unsigned long x)
{ return !(x & (x - 1)); }

// Returns the smallest power of 2 larger than x.
static __inline__ unsigned long nextPowerOf2(unsigned long x)
{ return 1UL << find_last_bit_set(x); }

}

#endif // BITOPS_H
