// Copyright (c) 2012-2013, Aptarism SA.
//
// All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
// * Neither the name of the University of California, Berkeley nor the
//   names of its contributors may be used to endorse or promote products
//   derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
#include <gtest/gtest.h>

#include "string_serializer.h"

namespace media_graph {

namespace {

template <typename T>
void GenericSerializationTest(const T &value) {
    StringSerializer serializer;
    EXPECT_TRUE(serializer.process(value));
    StringDeSerializer deSerializer(serializer.value());
    T result;
    EXPECT_TRUE(deSerializer.process(&result));
    EXPECT_EQ(result, value);
}

}  // namespace

TEST(StringSerializerTest, IntTest) {
    int value = 0xCAFECAFE;
    GenericSerializationTest(value);
    value = -1;
    GenericSerializationTest(value);
    value = 1234;
    GenericSerializationTest(value);
    value = 0;
    GenericSerializationTest(value);
}

TEST(StringSerializerTest, Int64Test) {
    int64_t value = 0xDEADBEAFCAFECAFE;
    GenericSerializationTest(value);
    value = -1;
    GenericSerializationTest(value);
    value = 0;
    GenericSerializationTest(value);
}

TEST(StringSerializerTest, BoolTest) {
    bool value = true;
    GenericSerializationTest(value);
    value = false;
    GenericSerializationTest(value);
}

TEST(StringSerializerTest, FloatTest) {
    float value = -3.1234e8f;
    GenericSerializationTest(value);
}

TEST(StringSerializerTest, DoubleTest) {
    double value = 3.1415;
    GenericSerializationTest(value);
}

TEST(StringSerializerTest, StringTest) {
    std::string value("Hello, world");
    GenericSerializationTest(value);

    // Test the empty string
    value = "";
    GenericSerializationTest(value);

    // Test weird values, including 0.
    for (int i = 0; i < 7; ++i) {
        value.push_back(i);
    }
    GenericSerializationTest(value);
}

}  // namespace media_graph
