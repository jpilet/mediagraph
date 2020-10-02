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
#ifndef MEDIAGRAPH_TYPE_VISITOR_H
#define MEDIAGRAPH_TYPE_VISITOR_H

namespace media_graph {
// A type visitor is used to express an operation that can be done on several
// types. The invoker of a visitor does not need to know at compile time on what
// type it operates. The type is resolved at runtime.
//
// This is used in particular to create a serializer for a NamedProperty.
// A NamedProperty can be of several type, not necessarily known at compile time.
// With a serializer object that derives from TypeConstVisitor, it is still
// possible to serialize the property.
class TypeConstVisitor {
public:
    virtual ~TypeConstVisitor() {}

    virtual bool process(const int& value) = 0;
    virtual bool process(const int64_t& value) = 0;
    virtual bool process(const bool& value) = 0;
    virtual bool process(const float& value) = 0;
    virtual bool process(const double& value) = 0;
    virtual bool process(const std::string& value) = 0;
};

class TypeVisitor {
public:
    virtual ~TypeVisitor() {}

    virtual bool process(int* value) = 0;
    virtual bool process(int64_t* value) = 0;
    virtual bool process(bool* value) = 0;
    virtual bool process(float* value) = 0;
    virtual bool process(double* value) = 0;
    virtual bool process(std::string* value) = 0;
};

}  // namespace media_graph

#endif  // MEDIAGRAPH_TYPE_VISITOR_H
