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
#include "binary_serializer.h"

#include <stdint.h>
#include <assert.h>

namespace media_graph {

bool BinarySerializer::process(const int &value) {
    serialized_value_.push_back((value >> 24) & 0xFF);
    serialized_value_.push_back((value >> 16) & 0xFF);
    serialized_value_.push_back((value >>  8) & 0xFF);
    serialized_value_.push_back((value >>  0) & 0xFF);
    return true;
}

bool BinaryDeSerializer::process(int *value) {
    if (serialized_value_.size() < 4) {
        return false;
    }

    *value = ((static_cast<unsigned char>(serialized_value_[0])) << 24)
        + ((static_cast<unsigned char>(serialized_value_[1])) << 16)
        + ((static_cast<unsigned char>(serialized_value_[2])) << 8)
        + ((static_cast<unsigned char>(serialized_value_[3])) << 0);
    serialized_value_.erase(0, 4);
    return true;
}

bool BinarySerializer::process(const int64_t &value) {
    for (int i = 0; i < 8; ++i) {
        serialized_value_.push_back((value >> ((7 - i) * 8)) & 0xFF);
    }
    return true;
}

bool BinaryDeSerializer::process(int64_t *value) {
    if (serialized_value_.size() < 8) {
        return false;
    }

    *value = 0;
    for (int i = 0; i < 8; ++i) {
        *value += int64_t(static_cast<unsigned char>(serialized_value_[i]))
            << ((7 - i) * 8);
    }
    serialized_value_.erase(0, 8);
    return true;
}

bool BinarySerializer::process(const bool &value) {
    serialized_value_.push_back(value ? 0xFF : 0);
    return true;
}

bool BinaryDeSerializer::process(bool *value) {
    if (serialized_value_.size() < 1) {
        return false;
    }

    *value = (serialized_value_[0] != 0);
    serialized_value_.erase(0, 1);
    return true;
}

bool BinarySerializer::process(const float &value) {
    union {
        int as_int;
        float as_float;
    } mixedValue;
    mixedValue.as_float = value;
    return process(mixedValue.as_int);
}

bool BinaryDeSerializer::process(float *value) {
    assert(sizeof(int) == sizeof(float));
    union {
        int as_int;
        float as_float;
    } mixedValue;
    if (process(&mixedValue.as_int)) {
        *value = mixedValue.as_float;
        return true;
    }
    return false;
}

bool BinarySerializer::process(const double &value) {
    union {
        int64_t as_int64_t;
        double as_double;
    } mixedValue;
    mixedValue.as_double = value;
    return process(mixedValue.as_int64_t);
}

bool BinaryDeSerializer::process(double *value) {
    assert(sizeof(int64_t) == sizeof(double));
    union {
        int64_t as_int64_t;
        double as_double;
    } mixedValue;
    if (process(&mixedValue.as_int64_t)) {
        *value = mixedValue.as_double;
        return true;
    }
    return false;
}

bool BinarySerializer::process(const std::string &value) {
    int length(value.size());
    process(length);
    serialized_value_.append(value);
    return true;
}

bool BinaryDeSerializer::process(std::string *value) {
    int length;
    if (!process(&length)) {
        return false;
    }
    *value = serialized_value_.substr(0, length);
    serialized_value_.erase(0, length);
    return true;
}

}  // namespace media_graph
