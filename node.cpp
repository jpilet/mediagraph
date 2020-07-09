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
#include "node.h"

#include <assert.h>

#include <sstream>
#include <iostream>

#include "graph.h"
#include "stream.h"
#include "stream_reader.h"

#ifdef MEDIAGRAPH_USE_EASY_PROFILER
    #include <easy/profiler.h>
#endif

namespace media_graph {

NodeBase::NodeBase() : graph_(nullptr), running_(false), stopping_(false) { }

NodeBase::~NodeBase() {
    detach();
}

bool NodeBase::start() {
    std::unique_lock<std::mutex> lock(stop_event_mutex_);
    if (running_) {
        return true;
    }

    if (!allPinsConnected()) {
        return false;
    }
    openAllStreams();
    openConnectedPins();
    running_ = true;
    return true;
}

void NodeBase::stop() {
    if (stopping_ || !running_) { return; }

    stopping_ = true;
    disconnectAllPins();
    if (running_) {
        running_ = false;
        //lock.unlock();

        closeAllStreams();

        pin_activity_.notify_all();
        stop_event_.notify_all();
    }
    stopping_ = false;
}

bool NodeBase::isRunning() const {
    return running_;
}

void NodeBase::waitForPinActivity() const {
    for (int i = 0; i < numInputPin(); ++i) {
        if (inputPin(i)->canRead()) {
            return;
        }
    }
    std::unique_lock<std::mutex> lock(pin_activity_mutex_);

#ifdef MEDIAGRAPH_USE_EASY_PROFILER
    EASY_BLOCK("waitForPinActivity()", profiler::colors::BlueGrey50);
#endif
    pin_activity_.wait(lock);
}

void NodeBase::waitUntilStopped() {
    if (!this->running_) {
        return;
    }
    std::unique_lock<std::mutex> lock(stop_event_mutex_);
    stop_event_.wait(lock, [this]{ return !this->running_; });
}

bool NodeBase::allPinsConnected() const {
    int num_pins = numInputPin();
    for (int i = 0; i < num_pins; ++i) {
        if (!constInputPin(i)->isConnected())
            return false;
    }
    return true;
}

bool NodeBase::allPinsConnectedAndOpen() const {
    int num_pins = numInputPin();
    for (int i = 0; i < num_pins; ++i) {
        const NamedStream* stream = constInputPin(i)->connectedStream();
        if (!stream || !stream->isOpen()) {
            return false;
        }
    }
    return true;
}

void NodeBase::disconnectAllPins() {
    int num_pins = numInputPin();
    for (int i = 0; i < num_pins; ++i) {
        inputPin(i)->disconnect();
    }
}

void NodeBase::openConnectedPins() {
    int num_pins = numInputPin();
    for (int i = 0; i < num_pins; ++i) {
        // Does nothing if not connected.
        inputPin(i)->openConnectedStream();
    }
}

void NodeBase::closeConnectedPins() {
    int num_pins = numInputPin();
     for (int i = 0; i < num_pins; ++i) {
        NamedStream *stream = inputPin(i)->connectedStream();
		if (stream) {
			stream->close();
		}
    }
}

void NodeBase::disconnectAllStreams() {
    int num_streams = numOutputStream();
    for (int i = 0; i < num_streams; ++i) {
        outputStream(i)->disconnectReaders();
    }
}

void NodeBase::openAllStreams() {
    int num_streams = numOutputStream();
    for (int i = 0; i < num_streams; ++i) {
        outputStream(i)->open();
    }
}

void NodeBase::closeAllStreams() {
    int num_streams = numOutputStream();
    for (int i = 0; i < num_streams; ++i) {
        outputStream(i)->close();
    }
}

bool NodeBase::setNameAndGraph(const std::string& new_name, Graph* new_graph) {
    if (this->graph_) {
        // already added.
        return false;
    }
    graph_ = new_graph;
    name_ = new_name;
    return true;
}

void NodeBase::detach() {
    if (graph_) {
        graph_->removeNode(name_);
        graph_ = nullptr;
        name_ = "";
    }
}

NamedStream* NodeBase::getOutputStreamByName(const std::string& name) {
    int num_streams = numOutputStream();
    for (int i = 0; i < num_streams; ++i) {
        NamedStream *stream = outputStream(i);
		assert(stream != 0);
        if (stream && (name == stream->streamName())) {
            return stream;
        }
    }
    return 0;
}

NamedPin* NodeBase::getInputPinByName(const std::string& name) {
    int num_pins = numInputPin();
    for (int i = 0; i < num_pins; ++i) {
        NamedPin *pin = inputPin(i);
		assert(pin != 0);
        if (pin && (name == pin->name())) {
            return pin;
        }
    }
    return 0;
}

ThreadedNodeBase::~ThreadedNodeBase() {
    //graph()->removeNode(this);
}

bool ThreadedNodeBase::start() {
    if (isRunning()) {
        return true;
    }

    if (!NodeBase::start()) {
        return false;
    }

    if (startThread()) {
        return true;
    } else {
        stop();
    }
    return false;
}

bool ThreadedNodeBase::startThread() {
    thread_must_quit_ = false;
    creating_thread_id_ = std::this_thread::get_id();
    if (thread_.start(threadEntryPoint, this)) {
        return true;
    }
    return false;
}

void ThreadedNodeBase::stop() {
    thread_must_quit_ = true;
    NodeBase::stop();
    if (creating_thread_id_ == std::this_thread::get_id()) {
      thread_.waitForTermination();
    }
}

bool ThreadedNodeBase::isRunning() const {
    return NodeBase::isRunning() && thread_.isRunning();
}

void ThreadedNodeBase::waitUntilStopped() {
    NodeBase::waitUntilStopped();
    if (creating_thread_id_ == std::this_thread::get_id()) {
      thread_.waitForTermination();
    }
}

void ThreadedNodeBase::threadEntryPoint(void *ptr) {
    ThreadedNodeBase* instance = static_cast<ThreadedNodeBase*>(ptr);

#ifdef MEDIAGRAPH_USE_EASY_PROFILER
    EASY_THREAD(instance->name().c_str());
#endif

    try {
        instance->threadMain();
    } catch (std::exception& e) {
        std::cerr << instance->name() << ": exception: "
            << e.what() << std::endl;
    }

    NodeBase* base = static_cast<NodeBase*>(instance);
    instance->thread_must_quit_ = true;
    base->stop();
}

}  // namespace media_graph

