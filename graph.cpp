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
#include "graph.h"

#include <assert.h>
#include <string>

#include "stream.h"
#include "stream_reader.h"

using std::string;

namespace media_graph {
Graph::Graph() : started_(false), stopping_(false) {
    addGetProperty("started", this, &Graph::isStarted);
}

bool Graph::addNode(const std::string& name, std::shared_ptr<NodeBase> node) {
    std::lock_guard<std::mutex> lock(mutex_);
    assert(node);
    if (lockedGetNodeByName(name)) {
        // We do not accept two nodes with the same name.
        return false;
    }

    nodes_[name] = node;
    node->setNameAndGraph(name, this);
    return true;
}

void Graph::removeNode(const std::string& name) {
    std::unique_lock<std::mutex> lock(mutex_);

    auto it = nodes_.find(name);
    if (it != nodes_.end()) {
        auto node = it->second;
        nodes_.erase(it);

        node->disconnectAllPins();
        node->disconnectAllStreams();

        // Make sure to unlock before "node" is destroyed.
        lock.unlock();
    }
}

std::shared_ptr<NodeBase> Graph::getNodeByName(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    return lockedGetNodeByName(name);
}

std::shared_ptr<NodeBase> Graph::lockedGetNodeByName(const std::string& name) {
    auto it = nodes_.find(name);
    if (it != nodes_.end()) { return it->second; }
    return std::shared_ptr<NodeBase>();
}

bool Graph::start() {
    if (isStarted()) { return true; }

    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
        if (!it->second->start()) {
            // TODO: give a meaningful error.
            lockedStop();  // stop potentially started nodes.
            return false;
        }
    }
    started_ = true;
    return true;
}

bool Graph::isStarted() const {
    for (auto it : nodes_) {
        if (it.second->isRunning()) { return true; }
    }
    return false;
}

void Graph::waitUntilStopped() const {
    for (auto it : nodes_) { it.second->waitUntilStopped(); }
}

void Graph::stop() {
    // Prevent deadlocks
    if (stopping_) { return; }

    std::lock_guard<std::mutex> lock(mutex_);
    stopping_ = true;
    lockedStop();
    stopping_ = false;
}

void Graph::lockedStop() {
    for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
        it->second->closeConnectedPins();
        it->second->stop();
    }
    started_ = false;
}

void Graph::clear() {
    stop();

    while (nodes_.size()) { removeNode(nodes_.begin()->first); }
}

bool Graph::connect(NamedStream* stream, NamedPin* pin) {
    if (!stream || !pin) { return false; }

    // We allow streams and pins not belonging to any node.
    assert(!stream->node() || stream->node()->graph() == this);
    assert(!pin->node() || pin->node()->graph() == this);

    return pin->connect(stream);
}

bool Graph::connect(std::shared_ptr<NodeBase> source, const std::string& streamName,
                    std::shared_ptr<NodeBase> dest, const std::string& pinName) {
    if (!source || !dest) { return false; }
    NamedStream* stream = source->getOutputStreamByName(streamName);
    NamedPin* pin = dest->getInputPinByName(pinName);
    return connect(stream, pin);
}

bool Graph::connect(const std::string& source_name, const std::string& streamName,
                    const std::string& dest_name, const std::string& pinName) {
    return connect(getNodeByName(source_name), streamName, getNodeByName(dest_name), pinName);
}

std::shared_ptr<NodeBase> Graph::node(int num) const {
    auto it = nodes_.begin();
    for (int i = 0; i < num; ++i) {
        if (it == nodes_.end()) { return nullptr; }
        ++it;
    }
    return it->second;
}

}  // namespace media_graph
