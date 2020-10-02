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

#include "property.h"
#include "types/type_definition.h"

namespace media_graph {
namespace {
    class ABC : public PropertyList {
    public:
        ABC() : a_("a", 0), b_("b"), c_("c") {
            addGetProperty("pi", this, &ABC::getPi);
            addGetSetProperty("d", this, &ABC::getD, &ABC::setD);
        }

        virtual int numProperty() const { return 3 + PropertyList::numProperty(); }
        virtual NamedProperty* property(int id) {
            switch (id) {
                case 0: return &a_;
                case 1: return &b_;
                case 2: return &c_;
            }
            return PropertyList::property(id - 3);
        }

        Property<int> a_;
        Property<int> b_;
        Property<int> c_;

        double getPi() const { return 3.1415; }

        int d;
        int getD() const { return d; }
        bool setD(const int& value) {
            d = value;
            return true;
        }
    };

}  // namespace

TEST(PropertyTest, BasicEnumeration) {
    ABC abc;
    PropertyList* propList = &abc;

    EXPECT_EQ(5, propList->numProperty());
    EXPECT_EQ(0, propList->property(5));
    EXPECT_EQ(&abc.a_, propList->property(0));
    EXPECT_EQ(&abc.b_, propList->property(1));
    EXPECT_EQ(&abc.c_, propList->property(2));
}

TEST(PropertyTest, IntNameAndTypeName) {
    Property<int> a("a", 0);
    EXPECT_EQ("int", a.typeName());
    EXPECT_EQ("a", a.name());
}

TEST(PropertyTest, IntGetSet) {
    Property<int> a("a", 0);
    EXPECT_EQ(0, a.get());
    a.set(0xDEADBEAF);
    EXPECT_EQ(0xDEADBEAF, a.get());

    Property<int> b("a", 0);
    a.set(3);
    std::string three = a.getSerialized();
    EXPECT_TRUE(b.setSerialized(three));
    EXPECT_EQ(3, b.get());

    a.set(0xDEADBEAF);
    std::string deadBeaf = a.getSerialized();
    EXPECT_TRUE(b.setSerialized(deadBeaf));
    EXPECT_EQ(0xDEADBEAF, b.get());
}

TEST(PropertyTest, ReadOnlyTest) {
    ReadOnlyProperty<int> a("a");
    ReadOnlyProperty<int> b("b", 0);

    EXPECT_FALSE(a.isWritable());
    EXPECT_FALSE(a.setSerialized(""));
}

TEST(PropertyTest, SimpleGetSet) {
    ABC object;

    EXPECT_EQ("pi", object.property(3)->name());
    EXPECT_EQ("d", object.property(4)->name());
}

}  // namespace media_graph
