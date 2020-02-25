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
#include "string_serializer.h"

#include <stdint.h>
#include <assert.h>
#include <sstream>

namespace media_graph {

template <typename T> std::string toString(T tmp)
{
    std::stringstream  sstr;
    sstr << tmp;
    return sstr.str();
}


template <typename T> T fromString(std::string strValue)
{
    T tmp;
    std::stringstream  sstr;
    sstr << strValue;
    sstr >> tmp;
    return tmp;
}


bool StringSerializer::process(const int &value) {
    serialized_value_ = toString(value);
    return true;
}

bool StringDeSerializer::process(int *value) {
    if (serialized_value_.size() < 1) {
        return false;
    }
    *value = fromString<int>(serialized_value_);
    return true;
}

bool StringSerializer::process(const int64_t &value) {
    serialized_value_ = toString(value);
    return true;
}

bool StringDeSerializer::process(int64_t *value) {
    if (serialized_value_.size() < 1) {
        return false;
    }
    *value = fromString<int64_t>(serialized_value_);
    return true;
}

bool StringSerializer::process(const bool &value) {
    serialized_value_ = toString(value);
    return true;
}

bool StringDeSerializer::process(bool *value) {
    if (serialized_value_.size() < 1) {
        return false;
    }
    *value = fromString<bool>(serialized_value_);
    return true;
}

bool StringSerializer::process(const float &value) {
    serialized_value_ = toString(value);
    return true;
}

bool StringDeSerializer::process(float *value) {
    if (serialized_value_.size() < 1) {
        return false;
    }
    *value = fromString<float>(serialized_value_);
    return true;
}

bool StringSerializer::process(const double &value) {
    serialized_value_ = toString(value);
    return true;
}

bool StringDeSerializer::process(double *value) {
    if (serialized_value_.size() < 1) {
        return false;
    }
    *value = fromString<double>(serialized_value_);
    return true;
}

bool StringSerializer::process(const std::string &value) {
    serialized_value_ = toString(value);
    return true;
}

bool StringDeSerializer::process(std::string *value) {
    *value = serialized_value_;
    return true;
}

}  // namespace media_graph
