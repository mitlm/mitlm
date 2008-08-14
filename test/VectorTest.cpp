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

typedef DenseVector<int>    IntVector;
typedef DenseVector<float>  FloatVector;
typedef DenseVector<double> DoubleVector;

////////////////////////////////////////////////////////////////////////////////

TEST(RangeTest, EndIndex) {
    Range r(3);
    EXPECT_EQ(0u, r.beginIndex());
    EXPECT_EQ(3u, r.endIndex());
    EXPECT_EQ(3u, r.length());
}

TEST(RangeTest, BeginEndIndex) {
    Range r(1, 3);
    EXPECT_EQ(1u, r.beginIndex());
    EXPECT_EQ(3u, r.endIndex());
    EXPECT_EQ(2u, r.length());
}

////////////////////////////////////////////////////////////////////////////////

TEST(VectorConstructorTest, Default) {
    IntVector v;
    EXPECT_EQ(0u, v.length());
}

TEST(VectorConstructorTest, Size) {
    IntVector v(3);
    EXPECT_EQ(3u, v.length());
}

TEST(VectorConstructorTest, SizeValue) {
    size_t N = 3;
    IntVector v(N, -1);
    ASSERT_EQ(N, v.length());
    for (size_t i = 0; i < N; ++i)
        EXPECT_EQ(-1, v[i]);
}

TEST(VectorConstructorTest, Range1) {
    size_t N = 3;
    IntVector v(Range(3));
    ASSERT_EQ(N, v.length());
    for (size_t i = 0; i < N; ++i)
        EXPECT_EQ((int) i, v[i]);
}

TEST(VectorConstructorTest, Range2) {
    size_t N = 3;
    IntVector v(Range(1, N));
    ASSERT_EQ(N - 1, v.length());
    for (size_t i = 0; i < N - 1; ++i)
        EXPECT_EQ((int) i + 1, v[i]);
}

TEST(VectorConstructorTest, DenseVector) {
    IntVector x(Range(3));
    IntVector y(x);
    ASSERT_EQ(3u, y.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int) i, y[i]);
}

// SparseVector

////////////////////////////////////////////////////////////////////////////////

TEST(VectorAssignmentTest, DenseVector) {
    size_t N = 3;
    IntVector x(Range(3)), y(3);
    y = x;
    ASSERT_EQ(N, y.length());
    for (size_t i = 0; i < N; ++i)
        EXPECT_EQ((int) i, y[i]);
}

TEST(VectorAssignmentTest, DenseVectorReset) {
    size_t N = 3;
    IntVector x(Range(3)), y;
    y = x;
    ASSERT_EQ(N, y.length());
    for (size_t i = 0; i < N; ++i)
        EXPECT_EQ((int) i, y[i]);
}

// SparseVector
// SparseVectorReset

////////////////////////////////////////////////////////////////////////////////

TEST(VectorResizingTest, Reset) {
    IntVector v(Range(3));

    v.reset(3);
    ASSERT_EQ(3u, v.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int) i, v[i]);

    v.reset(5);
    ASSERT_EQ(5u, v.length());

    v.reset(2);
    ASSERT_EQ(2u, v.length());
}

TEST(VectorResizingTest, ResetDefValue) {
    IntVector v(Range(3));

    v.reset(3, -1);
    ASSERT_EQ(3u, v.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ(-1, v[i]);

    v.reset(5, -1);
    ASSERT_EQ(5u, v.length());
    for (size_t i = 0; i < 5; ++i)
        EXPECT_EQ(-1, v[i]);

    v.reset(2, -1);
    ASSERT_EQ(2u, v.length());
    for (size_t i = 0; i < 2; ++i)
        EXPECT_EQ(-1, v[i]);
}

TEST(VectorResizingTest, Resize) {
    IntVector v(Range(3));

    ASSERT_EQ(3u, v.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int) i, v[i]);

    v.resize(5);
    ASSERT_EQ(5u, v.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int) i, v[i]);
}

TEST(VectorResizingTest, ResizeDefValue) {
    IntVector v(Range(3));

    ASSERT_EQ(3u, v.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int) i, v[i]);

    v.resize(5, -1);
    ASSERT_EQ(5u, v.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int) i, v[i]);
    for (size_t i = 3; i < 5; ++i)
        EXPECT_EQ(-1, v[i]);
}

////////////////////////////////////////////////////////////////////////////////

TEST(VectorIndexingTest, Simple1) {
    IntVector v(Range(3));
    v[0] = -1;
    EXPECT_EQ(-1, v[0]);
    EXPECT_EQ(1, v[1]);
    EXPECT_EQ(2, v[2]);
}

TEST(VectorIndexingTest, Simple2) {
    IntVector v(Range(3));
    v[2] = -1;
    EXPECT_EQ(0, v[0]);
    EXPECT_EQ(1, v[1]);
    EXPECT_EQ(-1, v[2]);
}

////////////////////////////////////////////////////////////////////////////////

TEST(OpAddTest, Scalar) {
    EXPECT_EQ(100005, (OpAdd::Eval(5, 100000)));
    EXPECT_DOUBLE_EQ(1.1, (OpAdd::Eval(1, 0.1)));
    EXPECT_FLOAT_EQ(1.1f, (OpAdd::Eval(1, 0.1f)));
}
////////////////////////////////////////////////////////////////////////////////

TEST(ScalarTest, Basic) {
    Scalar<int> s(5);
    EXPECT_EQ(5, (int)s);
}

TEST(ScalarTest, Iterator) {
    Scalar<int> s(5);
    Scalar<int>::ConstIterator begin = s.begin(), end = s.end();
    ASSERT_TRUE(begin == end);
    ASSERT_EQ(5, *++begin);
    ASSERT_EQ(5, *begin);
    ASSERT_TRUE(begin == end);
    ASSERT_EQ(5, *++end);
    ASSERT_EQ(5, *end);
    ASSERT_TRUE(begin == end);
}

////////////////////////////////////////////////////////////////////////////////

TEST(VectorClosureTest, ScalarScalar) {
    Scalar<int> a(5), b(10);
    VectorClosure<OpAdd, Scalar<int>, Scalar<int> > closure(a, b);

    EXPECT_EQ(1u, closure.length());
    EXPECT_TRUE(closure.begin() == closure.end());
    EXPECT_EQ(15, *closure.begin());
}

TEST(VectorClosureTest, ScalarVector) {
    Scalar<int> a(5);
    IntVector   x(Range(3));
    VectorClosure<OpAdd, Scalar<int>, IntVector> closure(a, x);

    EXPECT_EQ(3u, closure.length());
    EXPECT_TRUE(closure.begin() != closure.end());

    VectorClosure<OpAdd,Scalar<int>,IntVector>::ConstIterator p=closure.begin();
    EXPECT_EQ(5, *p);
    EXPECT_EQ(6, *++p);
    EXPECT_EQ(7, *++p);
    EXPECT_TRUE(++p == closure.end());
}

TEST(VectorClosureTest, VectorVector) {
    IntVector x(Range(3)), y(Range(3));
    VectorClosure<OpAdd, IntVector, IntVector> closure(x, y);

    EXPECT_EQ(3u, closure.length());
    EXPECT_TRUE(closure.begin() != closure.end());

    VectorClosure<OpAdd,IntVector,IntVector>::ConstIterator p = closure.begin();
    EXPECT_EQ(0, *p);
    EXPECT_EQ(2, *++p);
    EXPECT_EQ(4, *++p);
    EXPECT_TRUE(++p == closure.end());
}

////////////////////////////////////////////////////////////////////////////////

TEST(VectorEvalTest, Vector) {
    IntVector x(Range(1,4)), y;

    y = -x;
    ASSERT_EQ(3u, y.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int)(-(i+1)), y[i]);
}

TEST(VectorEvalTest, ConstantVector) {
    IntVector x(Range(1,4)), y;

    y = Scalar<int>(5) + x;
    ASSERT_EQ(3u, y.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int)(5 + (i+1)), y[i]);
}

TEST(VectorEvalTest, ScalarVector) {
    Scalar<int> a(5);
    IntVector x(Range(1,4)), y;

    y = a + x;
    ASSERT_EQ(3u, y.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int)(5 + (i+1)), y[i]);

    y = a - x;
    ASSERT_EQ(3u, y.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int)(5 - (i+1)), y[i]);

    y = a * x;
    ASSERT_EQ(3u, y.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int)(5 * (i+1)), y[i]);

    y = a / x;
    ASSERT_EQ(3u, y.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int)(5 / (i+1)), y[i]);
}

TEST(VectorEvalTest, VectorScalar) {
    Scalar<int> a(2);
    IntVector x(Range(1,4)), y;

    y = x + a;
    ASSERT_EQ(3u, y.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int)((i+1) + 2), y[i]);

    y = x - a;
    ASSERT_EQ(3u, y.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int)((i+1) - 2), y[i]);

    y = x * a;
    ASSERT_EQ(3u, y.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int)((i+1) * 2), y[i]);

    y = x / a;
    ASSERT_EQ(3u, y.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int)((i+1) / 2), y[i]);
}

TEST(VectorEvalTest, VectorVector) {
    IntVector x(Range(1,4)), y(Range(1,4)), z;

    z = x + y;
    ASSERT_EQ(3u, z.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int)(i+1)*2, z[i]);

    z = x - y;
    ASSERT_EQ(3u, z.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int)0, z[i]);

    z = x * y;
    ASSERT_EQ(3u, z.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int)((i+1)*(i+1)), z[i]);

    z = x / y;
    ASSERT_EQ(3u, z.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int)1, z[i]);

    ASSERT_EQ(14, dot(x, y));
}

TEST(VectorEvalTest, IndirectVector) {
    IntVector x(Range(3)), y(3), z(3);
    y[0] = 2; y[1] = 0; y[2] = 1;
    z = x[y];
    ASSERT_EQ(3u, z.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ(y[i], z[i]);
}

TEST(VectorEvalTest, InlineOperators) {
    IntVector x(3), y(3), z(3, 1);
    x[0] = 1; x[1] = 2; x[2] = 3;
    y[0] = 1; y[1] = 3; y[2] = 5;

    x += 1;
    EXPECT_EQ(3u, x.length());
    EXPECT_EQ(2, x[0]);
    EXPECT_EQ(3, x[1]);
    EXPECT_EQ(4, x[2]);

    x -= 1;
    EXPECT_EQ(3u, x.length());
    EXPECT_EQ(1, x[0]);
    EXPECT_EQ(2, x[1]);
    EXPECT_EQ(3, x[2]);

    x *= 2;
    EXPECT_EQ(3u, x.length());
    EXPECT_EQ(2, x[0]);
    EXPECT_EQ(4, x[1]);
    EXPECT_EQ(6, x[2]);

    x /= 2;
    EXPECT_EQ(3u, x.length());
    EXPECT_EQ(1, x[0]);
    EXPECT_EQ(2, x[1]);
    EXPECT_EQ(3, x[2]);

    x += y;
    EXPECT_EQ(3u, x.length());
    EXPECT_EQ(2, x[0]);
    EXPECT_EQ(5, x[1]);
    EXPECT_EQ(8, x[2]);

    z *= -y * 2;
    EXPECT_EQ(3u, z.length());
    EXPECT_EQ(-2, z[0]);
    EXPECT_EQ(-6, z[1]);
    EXPECT_EQ(-10, z[2]);
}

TEST(VectorEvalTest, BooleanOperators) {
    IntVector x(Range(3));
    IntVector y;

    y = (x == 1);
    ASSERT_EQ(3u, y.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ(i == 1, y[i]);

    y = (x != 1);
    ASSERT_EQ(3u, y.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ(i != 1, y[i]);
}

TEST(VectorEvalTest, AdvancedOperators) {
    IntVector x(Range(1, 4));
    FloatVector y(3);

    y = log(asFloat(x));
    ASSERT_EQ(3u, y.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_FLOAT_EQ(log((float)(i+1)), y[i]);

    y = log10(asFloat(x));
    ASSERT_EQ(3u, y.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_FLOAT_EQ(log10((float)(i+1)), y[i]);

    y = exp(asFloat(x));
    ASSERT_EQ(3u, y.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_FLOAT_EQ(exp((float)(i+1)), y[i]);

    y = pow(x, 2.0f);
    ASSERT_EQ(3u, y.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_FLOAT_EQ(pow((float)(i+1), 2.0f), y[i]);
}

TEST(VectorEvalTest, ComplexExpressions) {
    IntVector x(Range(3)), y(Range(3)), z;
    FloatVector w;
    z = 1 - (-x) - 1 + x + 2 * y / (1 + x);
    ASSERT_EQ(3u, z.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int)(1 - (-i) - 1 + i + 2 * i / (1 + i)), z[i]);

    z = (2 * x + 3)[y];
    ASSERT_EQ(3u, z.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int)(2*i + 3), z[i]);

    z = (2 * x + 3)[y][y];
    ASSERT_EQ(3u, z.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ((int)(2*i + 3), z[i]);

    w = log(asFloat(2 * x + 3))[y][y];
    ASSERT_EQ(3u, z.length());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_FLOAT_EQ(log(2*i + 3), w[i]);
}

TEST(VectorEvalTest, BinOps) {
    IntVector i(10), w(10), result(3, 0);
    i[0] = 0; i[1] = 0; i[2] = 2; i[3] = 1; i[4] = 1;
    i[5] = 2; i[6] = 0; i[7] = 1; i[8] = 2; i[9] = 0;
    w[0] = 1; w[1] = 1; w[2] = 2; w[3] = 0; w[4] = 0;
    w[5] = 2; w[6] = 1; w[7] = 0; w[8] = 1; w[9] = 3;

    BinCount(i, result);
    EXPECT_EQ(4, result[0]);
    EXPECT_EQ(3, result[1]);
    EXPECT_EQ(3, result[2]);

    result.set(0);
    BinWeight(i, w, result);
    EXPECT_EQ(6, result[0]);
    EXPECT_EQ(0, result[1]);
    EXPECT_EQ(5, result[2]);

    BinLookup(i, result, w);
    EXPECT_EQ(6, w[0]);
    EXPECT_EQ(6, w[1]);
    EXPECT_EQ(5, w[2]);
    EXPECT_EQ(0, w[3]);
    EXPECT_EQ(0, w[4]);
    EXPECT_EQ(5, w[5]);
    EXPECT_EQ(6, w[6]);
    EXPECT_EQ(0, w[7]);
    EXPECT_EQ(5, w[8]);
    EXPECT_EQ(6, w[9]);

    BinLookup(w, result, i, 3);
    EXPECT_EQ(3, i[0]);
    EXPECT_EQ(3, i[1]);
    EXPECT_EQ(3, i[2]);
    EXPECT_EQ(6, i[3]);
    EXPECT_EQ(6, i[4]);
    EXPECT_EQ(3, i[5]);
    EXPECT_EQ(3, i[6]);
    EXPECT_EQ(6, i[7]);
    EXPECT_EQ(3, i[8]);
    EXPECT_EQ(3, i[9]);
}

TEST(VectorEvalTest, Typecast) {
    IntVector i(Range(3));
    DoubleVector d;

    d = asDouble(i);
    ASSERT_EQ(3u, d.length());
    EXPECT_DOUBLE_EQ(0.0, d[0]);
    EXPECT_DOUBLE_EQ(1.0, d[1]);
    EXPECT_DOUBLE_EQ(2.0, d[2]);

    d += 0.1;
    i = asInt(d);
    ASSERT_EQ(3u, i.length());
    EXPECT_EQ(0, i[0]);
    EXPECT_EQ(1, i[1]);
    EXPECT_EQ(2, i[2]);
}

////////////////////////////////////////////////////////////////////////////////

TEST(VectorAssignTest, IndexAssign) {
    IntVector x(Range(3)), y(3, -1), m(2);
    m[0] = 0; m[1] = 2;

    IndexAssign(m, x, y);
    ASSERT_EQ(3u, y.length());
    EXPECT_EQ(0,  y[0]);
    EXPECT_EQ(-1, y[1]);
    EXPECT_EQ(2,  y[2]);

    IndexAssign(m, x, y, false);
    ASSERT_EQ(3u, y.length());
    EXPECT_EQ(0,  y[0]);
    EXPECT_EQ(1,  y[1]);
    EXPECT_EQ(2,  y[2]);
}

TEST(VectorAssignTest, MaskAssign) {
    IntVector x(Range(3)), y(3, -1), z(3, 5), f(3);
    f = (x != 1);
    EXPECT_EQ(1, f[0]);
    EXPECT_EQ(0, f[1]);
    EXPECT_EQ(1, f[2]);

    MaskAssign(f, x, y);
    ASSERT_EQ(3u, y.length());
    EXPECT_EQ(0,  y[0]);
    EXPECT_EQ(-1, y[1]);
    EXPECT_EQ(2,  y[2]);

    MaskAssign(f, x, y, false);
    ASSERT_EQ(3u, y.length());
    EXPECT_EQ(0,  y[0]);
    EXPECT_EQ(1,  y[1]);
    EXPECT_EQ(2,  y[2]);

    MaskAssign(f, x, z, y);
    ASSERT_EQ(3u, y.length());
    EXPECT_EQ(0,  y[0]);
    EXPECT_EQ(5,  y[1]);
    EXPECT_EQ(2,  y[2]);
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
