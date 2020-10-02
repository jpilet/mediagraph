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
#ifndef MEDIAGRAPH_PROPERTY_H
#define MEDIAGRAPH_PROPERTY_H

#include <string>
#include <vector>

#include "types/binary_serializer.h"
#include "types/string_serializer.h"
#include "types/type_definition.h"
#include "types/type_visitor.h"

namespace media_graph {
//! A named property with type unknown at compile time.
//! The property can be accessed through serialization or visitors.
//!
//! The recommended way of declaring a NamedProperty is best explained with an
//! example:
//!        class Foo : public PropertyList {
//!          public:
//!            // We have accessors for the "bar" property.
//!            int getBar() const;
//!            bool setBar(int& bar);
//!
//!            void addProperties() {
//!                 addGetSetProperty("bar", this, Foo::getBar, Foo::setBar);
//!                 addGetProperty("foobar", this, Foo::getFooBar);
//!            }
//!
//!            // We declare the property in all constructors
//!            Foo() { addProperties(); }
//!            Foo(const Foo& a) : PropertyList(a) { addProperties(); }
//!         };
//!
//! If, anyway, you just declare a member and you do not do anything special
//! in the accessors, you can also use the Property class:
//!     class Object : public PropertyList {
//!       public:
//!         Object() : myProperty_("My Property", 0) { }
//!         virtual int numProperty() const { return PropertyList::numProperty() + 1; }
//!         virtual NamedProperty* property(int id) {
//!            switch(id) { case 0: return &myProperty_; }
//!            return PropertyList::property(id - 1);
//!         }
//!       private:
//!         Property<int> myProperty_;
//!     };
class NamedProperty {
public:
    NamedProperty(const std::string& name) : name_(name) {}
    virtual ~NamedProperty() {}

    //! Returns the property name.
    const std::string& name() const { return name_; }

    //! Returns a string describing the property type.
    virtual std::string typeName() const = 0;

    virtual bool isWritable() const { return true; }

    //! Apply a read-only visitor.
    virtual bool apply(TypeConstVisitor* operation) const = 0;

    //! Apply a visitor and let it write to the property.
    virtual bool apply(TypeVisitor* operation) = 0;

    //! Returns the value as string
    std::string ValueToString() {
        StringSerializer serializer;
        apply(&serializer);
        return serializer.value();
    }

    //! Sets the property to the value string serialized.
    //! Returns true on success. Otherwise, returns false.
    bool ValueFromString(const std::string& serialized) {
        StringDeSerializer deSerializer(serialized);
        return apply(&deSerializer);
    }

    //! Returns a the property serialized value.
    std::string getSerialized() const {
        BinarySerializer serializer;
        apply(&serializer);
        return serializer.value();
    }

    //! Sets the property to the value serialized in <serialized>.
    //! Returns true on success. Otherwise, returns false. In that case,
    //! the property value is not modified.
    bool setSerialized(const std::string& serialized) {
        BinaryDeSerializer deSerializer(serialized);
        return apply(&deSerializer);
    }

private:
    std::string name_;
};

//! Common interface for a typed property. Inherited by \see Property and
//! GetSetProperty.
template <class T> class PropertyInterface : public NamedProperty {
public:
    PropertyInterface(const std::string& name) : NamedProperty(name) {}

    virtual std::string typeName() const { return media_graph::typeName<T>(); }

    virtual T get() const = 0;
    virtual bool set(const T& value) = 0;

    virtual bool apply(TypeConstVisitor* operation) const { return operation->process(get()); }

    virtual bool apply(TypeVisitor* operation) {
        T temporary = get();
        bool result = operation->process(&temporary);
        if (get() != temporary) { set(temporary); }
        return result;
    }
};

//! A typed and named property.
//! @see NamedProperty for general usage explanation.
template <class T> class Property : public PropertyInterface<T> {
public:
    Property(const std::string& name) : PropertyInterface<T>(name) {}
    Property(const std::string& name, const T& value) : PropertyInterface<T>(name), value_(value) {}

    virtual T get() const { return value_; }
    virtual bool set(const T& value) {
        value_ = value;
        return true;
    }

    T& getMutable() { return value_; }

    virtual bool apply(TypeVisitor* operation) { return operation->process(&value_); }

    Property<T>& operator=(const T& value) {
        set(value);
        return *this;
    }

    operator T() const { return value_; }

private:
    T value_;
};

//! A read only property.
//! @see Property
template <class T> class ReadOnlyProperty : public Property<T> {
public:
    ReadOnlyProperty(const std::string& name) : Property<T>(name) {}
    ReadOnlyProperty(const std::string& name, const T& value) : Property<T>(name, value) {}

    virtual bool isWritable() const { return false; }
    virtual bool setSerialized(const std::string& /*serialized*/) { return false; }

    ReadOnlyProperty<T>& operator=(const T& value) {
        Property<T>::set(value);
        return *this;
    }

    operator T() const { return Property<T>::get(); }
};

//! Allows to define a read-only property from get method.
//! Do not instanciate this class directly. Instead,
//! \see PropertyList::addGetProperty
template <class Property_t, class Host_t> class GetProperty : public PropertyInterface<Property_t> {
public:
    typedef Property_t (Host_t::*GetMethod_t)() const;

    GetProperty(const std::string& name, Host_t* instance, const GetMethod_t get)
        : PropertyInterface<Property_t>(name), instance_(instance), getMethod_(get) {}

    virtual Property_t get() const { return ((*instance_).*getMethod_)(); }

    virtual bool isWritable() const { return false; }

    virtual bool set(const Property_t&) { return false; }

private:
    Host_t* instance_;
    GetMethod_t getMethod_;
};

//! Allows to define a property from get and set methods.
//! Do not instanciate this class directly. Instead,
//! \see PropertyList::addGetSetProperty
template <class Property_t, class Host_t>
class GetSetProperty : public PropertyInterface<Property_t> {
public:
    typedef Property_t (Host_t::*GetMethod_t)() const;
    typedef bool (Host_t::*SetMethod_t)(const Property_t&);

    GetSetProperty(const std::string& name, Host_t* instance, const GetMethod_t get,
                   const SetMethod_t set)
        : PropertyInterface<Property_t>(name),
          instance_(instance),
          getMethod_(get),
          setMethod_(set) {}

    virtual Property_t get() const { return ((*instance_).*getMethod_)(); }

    virtual bool set(const Property_t& value) { return ((*instance_).*setMethod_)(value); }

private:
    Host_t* instance_;
    GetMethod_t getMethod_;
    SetMethod_t setMethod_;
};

//! Base class for classes exposing properties.
//! \see NamedProperty for examples.
class PropertyList {
public:
    virtual ~PropertyList();
    PropertyList() {}

    //! The copy constructor does not copy properties, because properties are
    //! instance related anyway.
    PropertyList(const PropertyList& /*a*/) {}

    //! Returns the number of properties exposed by the object.
    //! Deriving classes must re-implement this method to expose properties.
    virtual int numProperty() const { return properties_.size(); }

    //! Get a pointer to a property. Returns 0 if the property id is not valid.
    //! Deriving classes must re-implement this method to expose properties.
    virtual NamedProperty* property(int id);

    //! The const version of getProperty.
    NamedProperty* getPropertyConst(int id) const {
        return const_cast<PropertyList*>(this)->property(id);
    }

    //! Get a pointer to a property by its name. Returns null if no property has this name.
    virtual NamedProperty* getPropertyByName(const std::string& name);

    //! Declares a new property with a get and a set method.
    //! \param name the property name.
    //! \param instance a pointer to the instance on which get/set should be
    //!                 called.
    //! \param get A pointer to the get method (Example: MyClass::getVar)
    //! \param set A pointer to the set method.
    template <class Property_t, class Host_t>
    void addGetSetProperty(const std::string& name, Host_t* instance,
                           Property_t (Host_t::*get)() const,
                           bool (Host_t::*set)(const Property_t&)) {
        properties_.push_back(new GetSetProperty<Property_t, Host_t>(name, instance, get, set));
    }

    template <class Property_t, class Host_t>
    void addGetProperty(const std::string& name, Host_t* instance,
                        Property_t (Host_t::*get)() const) {
        properties_.push_back(new GetProperty<Property_t, Host_t>(name, instance, get));
    }

private:
    std::vector<NamedProperty*> properties_;
};

}  // namespace media_graph

#endif  // MEDIAGRAPH_PROPERTY_H
