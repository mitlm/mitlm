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

#include <gtest/gtest.h>
#include "vector/DenseVector.h"
#include "vector/VectorOps.h"
#include "Timer.h"

#define MINITER 10
#define MAXITER 10000000

typedef int TYPE;
typedef DenseVector<TYPE>   TYPEVector;
typedef DenseVector<int>    IntVector;
typedef DenseVector<float>  FloatVector;
typedef DenseVector<double> DoubleVector;

////////////////////////////////////////////////////////////////////////////////

TEST(VectorTimeTest, VectorScalar) {
    printf("         N\t      C\t  CIter\t\tCVector\n");
    for (size_t N = MINITER; N <= MAXITER; N *= 10) {
        Range r(N);
        TYPEVector x(r), y(r), u(r), v(r), w(r);
        TYPEVector z(N, 0);

        START_TIME_N(N) {
            for (size_t i = 0; i < N; i++)
                z[i] = x[i] + 1;
        } END_TIME(timeC)

        START_TIME_N(N) {
            z = x + 1;
        } END_TIME(timeVector)

        START_TIME_N(N) {
            TYPE *px = x.begin(), *pz = z.begin(), *py = y.begin(),
                *pu = u.begin(), *pv = v.begin(), *pw = w.begin(),
                *pzEnd = z.end();
            while (pz != pzEnd) {
                *pz = *pz + 1;
                pz++; px++; py++; pu++; pv++; pw++;
            }
        } END_TIME(timeCIter)

        START_TIME_N(N) {
            z = x + 1;
        } END_TIME(timeVector2)

        printf("%10lu\t%7.2f\t%7.2f\t%7.2f\t%7.2f\n",
               (unsigned long)N, timeC, timeCIter, timeVector, timeVector2);

    }
    printf("Done\n");
}

TEST(VectorTimeTest, VectorVector) {
    printf("         N\t      C\t  CIter\t\tCVector\n");
    for (size_t N = MINITER; N <= MAXITER; N *= 10) {
        Range r(N);
        TYPEVector x(r), y(r);
        TYPEVector z(N, 0);

        START_TIME_N(N) {
            for (size_t i = 0; i < N; i++)
                z[i] = x[i] + y[i];
        } END_TIME(timeC)

        START_TIME_N(N) {
            z = x + y;
        } END_TIME(timeVector)

        START_TIME_N(N) {
            TYPE *px = x.begin(), *py = y.begin(), *pz = z.begin();
            TYPE *pzEnd = z.end();
            while (pz != pzEnd)
                *pz++ = *px++ + *py++;
        } END_TIME(timeCIter)

        START_TIME_N(N) {
            z = x + y;
        } END_TIME(timeVector2)

        printf("%10lu\t%7.2f\t%7.2f\t%7.2f\t%7.2f\n",
               (unsigned long)N, timeC, timeCIter, timeVector, timeVector2);

    }
    printf("Done\n");
}

TEST(VectorTimeTest, IndirectVector) {
    printf("         N\t      C\t  CIter\t\tCVector\n");
    for (size_t N = MINITER; N <= MAXITER; N *= 10) {
        Range r(N);
        IntVector y(N);
        TYPEVector x(r), z(N, 0);

        for (size_t i = 0; i < N; i++)
            y[i] = rand() % N;

        START_TIME_N(N) {
            for (size_t i = 0; i < N; i++)
                z[i] = x[y[i]];
        } END_TIME(timeC)

        START_TIME_N(N) {
            z = x[y];
        } END_TIME(timeVector)

        START_TIME_N(N) {
            TYPE *py = y.begin(), *pz = z.begin(), *pzEnd = z.end();
            while (pz != pzEnd)
                *pz++ = x[*py++];
        } END_TIME(timeCIter)

        START_TIME_N(N) {
            z = x[y];
        } END_TIME(timeVector2)

        printf("%10lu\t%7.2f\t%7.2f\t%7.2f\t%7.2f\n",
               (unsigned long)N, timeC, timeCIter, timeVector, timeVector2);

    }
    printf("Done\n");
}

TEST(VectorTimeTest, AdvancedOperators) {
    printf("         N\t      C\t  CIter\t\tCVector\n");
    for (size_t N = MINITER; N <= MAXITER; N *= 10) {
        Range r(N);
        FloatVector x(r), z(N, 0);


        START_TIME_N(N) {
            for (size_t i = 0; i < N; i++)
                z[i] = log(x[i]);
        } END_TIME(timeC)

        START_TIME_N(N) {
            z = log(x);
        } END_TIME(timeVector)

        START_TIME_N(N) {
            float *px = x.begin(), *pz = z.begin();
            float *pzEnd = z.end();
            while (pz != pzEnd)
                *pz++ = log(*px++);
        } END_TIME(timeCIter)

        START_TIME_N(N) {
            z = log(x);
        } END_TIME(timeVector2)

        printf("%10lu\t%7.2f\t%7.2f\t%7.2f\t%7.2f\n",
               (unsigned long)N, timeC, timeCIter, timeVector, timeVector2);

    }
    printf("Done\n");
}

TEST(VectorTimeTest, ComplexExpression1) {
    printf("         N\t      C\t  CIter\t\tCVector\n");
    for (size_t N = MINITER; N <= MAXITER; N *= 10) {
        Range r(N);
        FloatVector x(r), y(N);
        FloatVector z(N, 0);

        for (size_t i = 0; i < N; i++)
            y[i] = rand() % N;

        START_TIME_N(N) {
            for (size_t i = 0; i < N; i++)
                z[i] = x[i]*y[i] + y[i]/2 + 5;
        } END_TIME(timeC)

        START_TIME_N(N) {
            z = x*y + y/2 + 5;
        } END_TIME(timeVector)

        START_TIME_N(N) {
            float *px = x.begin(), *py = y.begin(),
                  *pz = z.begin(), *pzEnd = z.end();
            while (pz != pzEnd) {
                *pz = (*px)*(*py) + (*py)/2 + 5;
                py++; px++; pz++;
            }
        } END_TIME(timeCIter)

        START_TIME_N(N) {
            z = x*y + y/2 + 5;
        } END_TIME(timeVector2)

        printf("%10lu\t%7.2f\t%7.2f\t%7.2f\t%7.2f\n",
               (unsigned long)N, timeC, timeCIter, timeVector, timeVector2);

    }
    printf("Done\n");
}

TEST(VectorTimeTest, ComplexExpression2) {
    printf("         N\t      C\t  CIter\t\tCVector\n");
    for (size_t N = MINITER; N <= MAXITER; N *= 10) {
        Range r(N);
        FloatVector x(r), y(N), u(r), v(r);
        FloatVector z(N, 0);

        for (size_t i = 0; i < N; i++)
            y[i] = rand() % N;

        START_TIME_N(N) {
            for (size_t i = 0; i < N; i++)
                z[i] = x[i]*y[i] + u[i]/2 - log(v[i]);
        } END_TIME(timeC)

        START_TIME_N(N) {
            z = x*y + u/2 - log(v);
        } END_TIME(timeVector)

        START_TIME_N(N) {
            float *px = x.begin(), *py = y.begin(), *pz = z.begin(),
                  *pu = u.begin(), *pv = v.begin(), *pzEnd = z.end();
            while (pz != pzEnd) {
                *pz++ = (*px)*(*py) + (*pu)/2 - log(*pv);
                py++; px++; pu++; pv++;
            }
        } END_TIME(timeCIter)

        START_TIME_N(N) {
            z = x*y + u/2 - log(v);
        } END_TIME(timeVector2)

        printf("%10lu\t%7.2f\t%7.2f\t%7.2f\t%7.2f\n",
               (unsigned long)N, timeC, timeCIter, timeVector, timeVector2);

    }
    printf("Done\n");
}

TEST(VectorTimeTest, IndexAssign) {
    printf("         N\t      C\t  CIter\t\tCVector\n");
    for (size_t N = MINITER; N <= MAXITER; N *= 10) {
        Range r(N);
        TYPEVector x(r), z(N);
        IntVector m(N/10);

        for (size_t i = 0; i < m.length(); i++)
            m[i] = i % m.length();

        START_TIME_N(N) {
            for (size_t i = 0; i < m.length(); i++)
                z[m[i]] = x[m[i]]*x[m[i]] + 2;
        } END_TIME(timeC)

        START_TIME_N(N) {
            IndexAssign(m, x*x + 2, z);
        } END_TIME(timeVector)

        START_TIME_N(N) {
            int *pm = m.begin(), *pmEnd = m.end();
            while (pm != pmEnd) {
                z[*pm] = x[*pm]*x[*pm] + 2;
                pm++;
            }
        } END_TIME(timeCIter)

        START_TIME_N(N) {
            IndexAssign(m, x*x + 2, z);
        } END_TIME(timeVector2)

        printf("%10lu\t%7.2f\t%7.2f\t%7.2f\t%7.2f\n",
               (unsigned long)N, timeC, timeCIter, timeVector, timeVector2);

    }
    printf("Done\n");
}

TEST(VectorTimeTest, CondAssign1) {
    printf("         N\t      C\t  CIter\t\tCVector\n");
    for (size_t N = MINITER; N <= MAXITER; N *= 10) {
        Range r(N);
        TYPEVector x(r), z(N);
        IntVector m(N);
        for (size_t i = 0; i < N; i++)
            m[i] = (i % 100 == 0);


        START_TIME_N(N) {
            for (size_t i = 0; i < N; i++)
                if (m[i])
                    z[i] = x[i]*x[i] + 2;
        } END_TIME(timeC)

        START_TIME_N(N) {
            MaskAssign(m, x*x + 2, z);
        } END_TIME(timeVector)

        START_TIME_N(N) {
            TYPE *px = x.begin(), *pz = z.begin();
            int *pc = m.begin(), *pcEnd = m.end();
            while (pc != pcEnd) {
                if (*pc)
                    *pz = *px * *px + 2;
                ++px; ++pz; ++pc;
            }
        } END_TIME(timeCIter)

        START_TIME_N(N) {
            MaskAssign(m, x*x + 2, z);
        } END_TIME(timeVector2)

        printf("%10lu\t%7.2f\t%7.2f\t%7.2f\t%7.2f\n",
               (unsigned long)N, timeC, timeCIter, timeVector, timeVector2);

    }
    printf("Done\n");
}

TEST(VectorTimeTest, CondAssign2) {
    printf("         N\t      C\t  CIter\t\tCVector\n");
    for (size_t N = MINITER; N <= MAXITER; N *= 10) {
        Range r(N);
        TYPEVector x(r), y(N, 0), z(N);
        IntVector m(N);
        for (size_t i = 0; i < N; i++)
            m[i] = (i % 2 == 0);


        START_TIME_N(N) {
            for (size_t i = 0; i < N; i++)
                if (m[i])
                    z[i] = x[i]*x[i] + 2;
                else
                    z[i] = y[i]*y[i] + 2;
        } END_TIME(timeC)

        START_TIME_N(N) {
            MaskAssign(m, x*x + 2, y*y + 2, z);
        } END_TIME(timeVector)

        START_TIME_N(N) {
            TYPE *px = x.begin(), *py = y.begin(), *pz = z.begin();
            int *pc = m.begin(), *pcEnd = m.end();
            while (pc != pcEnd) {
                if (*pc)
                    *pz = *px * *px + 2;
                else
                    *pz = *py * *py + 2;;
                ++px; ++py; ++pz; ++pc;
            }
        } END_TIME(timeCIter)

        START_TIME_N(N) {
            MaskAssign(m, x*x + 2, y*y + 2, z);
        } END_TIME(timeVector2)

        printf("%10lu\t%7.2f\t%7.2f\t%7.2f\t%7.2f\n",
               (unsigned long)N, timeC, timeCIter, timeVector, timeVector2);

    }
    printf("Done\n");
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
