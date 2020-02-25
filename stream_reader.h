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
#ifndef MEDIAGRAPH_STREAM_READER_H
#define MEDIAGRAPH_STREAM_READER_H

#include <string>

#include "stream.h"
#include "node.h"
#include "property.h"

namespace media_graph {

class NodeBase;

/*! Type-agnostic plug to any StreamBase. Has a name.
 */
class NamedPin : public PropertyList {
  public:
    NamedPin(const std::string& name, NodeBase *node) : name_(name), node_(node) { }
    virtual ~NamedPin() { }
    const std::string& name() const { return name_; }

    virtual std::string typeName() const = 0;
    virtual bool connect(NamedStream* stream) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual NamedStream *connectedStream() const = 0;
    virtual bool canRead() const = 0;
    virtual void openConnectedStream() = 0;
    virtual void closeConnectedStream() = 0;

    NodeBase* node() const { return node_; }

    // This is called by the connected stream when new data arrive.
    void signalActivity() const { if (node_) node_->signalActivity(); }

    SequenceId lastReadSequenceId() const { return last_read_sequence_id_; }
  protected:
    SequenceId last_read_sequence_id_;

  private:
    std::string name_;
    NodeBase* node_;
};

/*! Nodes in the media graph read data from streams through a StreamReader.
 *  The node has to provide a name. It can then use read and tryRead.
 *  Of course, if Graph::connect() has not been called properly, reading will
 *  fail.
 */
template <typename T>
class StreamReader : public NamedPin {
  public:
    StreamReader(const std::string& name, NodeBase *node);
    virtual ~StreamReader();

    bool read(T* data, Timestamp* timestamp, SequenceId* seq = 0);
    bool tryRead(T* data, Timestamp* timestamp, SequenceId* seq = 0);
    virtual bool canRead() const;

    /*! Skip frames until reaching <timestamp>. Frames with a timestamp
     *  equal or lower than <timestamp> are to be ignored.
     */
    bool seek(Timestamp timestamp);
    Timestamp seekPosition() const { return seek_; }

    virtual std::string typeName() const;

    virtual bool connect(NamedStream* stream);
    virtual void disconnect();

    virtual void openConnectedStream() { if (isConnected()) pointer_->open(); }
    virtual void closeConnectedStream() { if (isConnected()) pointer_->close(); }

    virtual NamedStream *connectedStream() const { return pointer_; }
    virtual void open() { if (isConnected()) pointer_->open(); }
    virtual void close() { if (isConnected()) pointer_->close(); }

    virtual bool isConnected() const { return pointer_ != 0; }

    StreamBase<T>* get() { return pointer_; }

    // Public, but should only be accessed by classes inheriting StreamBase<T>.
    SequenceId* lastReadSequenceIdPtr() { return &last_read_sequence_id_; }

  private:
    StreamBase<T>* pointer_;
    Timestamp seek_;
};

template <typename T>
StreamReader<T>::StreamReader(const std::string& name, NodeBase* node)
    : NamedPin(name, node) {
    pointer_ = 0;
    seek_ = Timestamp::microSecondsSince1970(0);
}

template <typename T>
StreamReader<T>::~StreamReader() { disconnect(); }

template <typename T>
bool StreamReader<T>::read(T* data, Timestamp* timestamp, SequenceId* seq) {
    return (pointer_ && pointer_->read(this, data, timestamp, seq));
}

template <typename T>
bool StreamReader<T>::tryRead(T* data, Timestamp* timestamp, SequenceId* seq) {
    return (pointer_ && pointer_->tryRead(this, data, timestamp, seq));
}

template <typename T>
bool StreamReader<T>::canRead() const {
    return pointer_ && pointer_->canRead(last_read_sequence_id_, seek_);
}

template <typename T>
std::string StreamReader<T>::typeName() const {
    return media_graph::typeName<T>();
}

template <typename T>
void StreamReader<T>::disconnect() {
    if (pointer_) {
        // We make sure isConnected() reports false
        // before unregistering.
        StreamBase<T>* pointer_copy = pointer_;
        pointer_ = 0;
        pointer_copy->unregisterReader(this);
        if (node()) {
            node()->stop();
        }
    }
}

template <typename T>
bool StreamReader<T>::connect(NamedStream* stream) {
    if (this == 0) return false;
    disconnect();
    if (typeName() == stream->typeName()) {
        pointer_ = dynamic_cast<StreamBase<T>* >(stream);
        if (pointer_) {
            pointer_->registerReader(this);
            last_read_sequence_id_ = -1;
        }
    }
    return pointer_ != 0;
}

template <typename T>
bool StreamReader<T>::seek(Timestamp timestamp) {
    if (!(timestamp < seek_)) {
        seek_ = timestamp;
        return true;
    }
    return false;
}

}  // namespace media_graph

#endif  // MEDIAGRAPH_STREAM_READER_H
