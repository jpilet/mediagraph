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

Graph::Graph() : started_(false) {
    addGetProperty("started", this, &Graph::isStarted);
}

bool Graph::addNode(const std::string& name, NodeBase* node) {
    ScopedLock lock(&mutex_);
    assert(node != 0);
    if (lockedGetNodeByName(name) != 0) {
        // We do not accept two nodes with the same name.
        return false;
    }

    nodes_[name] = node;
    return true;
}

void Graph::removeNode(NodeBase* node) {
    ScopedLock lock(&mutex_);

    std::map<string, NodeBase*>::iterator it = nodes_.find(node->name());
    if (it != nodes_.end()) {
        it->second->disconnectAllPins();
        it->second->disconnectAllStreams();
        nodes_.erase(it);
    }
}

NodeBase* Graph::getNodeByName(const std::string& name) {
    ScopedLock lock(&mutex_);
    return lockedGetNodeByName(name);
}

NodeBase* Graph::lockedGetNodeByName(const std::string& name) {
    std::map<string, NodeBase*>::iterator it = nodes_.find(name);
    if (it != nodes_.end()) {
        return it->second;
    }
    return 0;
}

bool Graph::start() {
    if (isStarted()) {
        return true;
    }

    ScopedLock lock(&mutex_);
    for (std::map<string, NodeBase*>::iterator it = nodes_.begin();
         it != nodes_.end(); ++it) {
        if (!it->second->start()) {
            // TODO: give a meaningful error.
            lockedStop();  // stop potentially started nodes.
            return false;
        }
    }
    started_ = true;
    return true;
}

void Graph::stop() {
    ScopedLock lock(&mutex_);
    lockedStop();
}

void Graph::lockedStop() {
    for (std::map<string, NodeBase*>::iterator it = nodes_.begin();
         it != nodes_.end(); ++it) {
        it->second->closeConnectedPins();
        it->second->stop();
    }
}

void Graph::clear() {
    stop();

    while (nodes_.size() > 0) {
        delete nodes_.begin()->second;
    }
}

bool Graph::connect(NamedStream* stream, NamedPin* pin) {
    if (!stream || !pin) {
        return false;
    }

    // We allow streams and pins not belonging to any node.
    assert(stream->node() == 0 || stream->node()->graph() == this);
    assert(pin->node() == 0 || pin->node()->graph() == this);
 
    return pin->connect(stream);
}

bool Graph::connect(NodeBase* source, const std::string& streamName,
                    NodeBase* dest, const std::string& pinName) {
    if (!source || !dest) {
        return false;
    }
    NamedStream* stream = source->getOutputStreamByName(streamName);
    NamedPin* pin = dest->getInputPinByName(pinName);
    return connect(stream, pin);
}

bool Graph::connect(const std::string& source_name, const std::string& streamName,
                    const std::string& dest_name, const std::string& pinName) {
    return connect(getNodeByName(source_name), streamName,
                   getNodeByName(dest_name), pinName);
}

NodeBase* Graph::node(int num) const {
    std::map<std::string, NodeBase*>::const_iterator it = nodes_.begin();
    for (int i = 0; i < num; ++i) {
        if (it == nodes_.end()) {
            return 0;
        }
        ++it;
    }
    return it->second;
}

}  // namespace media_graph
