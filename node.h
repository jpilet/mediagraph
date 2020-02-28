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
#ifndef MEDIAGRAPH_NODE_H
#define MEDIAGRAPH_NODE_H

#include <string>

#include "thread_primitives.h"
#include "property.h"

namespace media_graph {

class Graph;
class NamedStream;
class NamedPin;

/*! All nodes in the media graph derive from this class. Basically, it
 * provides reflection on pins.
 */
class NodeBase : public PropertyList {
  public:
    NodeBase();

    virtual ~NodeBase();

    /// Tries to start the node.
    /// The default implementation verifies that all input pins are connected.
    /// It calls 'open()' on all output streams and all pins.
    /// Inheriting nodes can add more verification & initialization code by
    /// overloading this method.
    virtual bool start();

    /// Stops the node and disconnects all connected pins.
    /// Disconnecting all pins is necessary to make sure the graph can continue
    /// to run even if the node is stopped.
    virtual void stop();

    /// Returns true if start() has been called successfully.
    virtual bool isRunning() const;

    /// Returns the number of output streams that your node exposes.
    /// Any node exposing at least one output stream must overload this method.
    virtual int numOutputStream() const { return 0; }

    /// Returns a pointer to outputStream number <index>, or 0 if index is out
    /// of range. 
    virtual const NamedStream* constOutputStream(int index) const { return 0; }

    NamedStream* outputStream(int index) {
        return const_cast<NamedStream*>(this->constOutputStream(index));
    }

    /// Gets a stream by its name. Returns null if no stream has this name.
    virtual NamedStream* getOutputStreamByName(const std::string& name);

    /// Returns the number of input pins. Any node with at least one input pins
    /// must overload this method.
    virtual int numInputPin() const { return 0; } 

    /// Returns a pointer to input pin number <index>, or null if index is out
    /// of range. Nodes with input pins must overload this method.
    virtual const NamedPin* constInputPin(int index) const { return 0; };
    NamedPin* inputPin(int index) {
        return const_cast<NamedPin*>(this->constInputPin(index));
    }

    /// Returns the pin named <name>, or null if there is no such pin.
    virtual NamedPin* getInputPinByName(const std::string& name);

    /// Wait for any input pin to receive new data. To know which one, iterate
    /// call tryRead on all input pins.
    void waitForPinActivity();

    bool allPinsConnected() const;
    void openConnectedPins();
    void closeConnectedPins();
    void disconnectAllPins();
    void disconnectAllStreams();
    void openAllStreams();
    void closeAllStreams();

    void signalActivity() { pin_activity_.notify_all(); }

    const std::string& name() const { return name_; }
    Graph* graph() const { return graph_; }

    /// Sets the node membership to a graph. Called by Graph::addNode() only.
    /// Fails if if the node is already part of a graph.
    bool setNameAndGraph(const std::string& name, Graph* graph);

    // unplug the node from the graph.
    void detach();

  private:
    std::condition_variable pin_activity_;
    std::mutex pin_activity_mutex_;
    
    Graph* graph_;
    std::string name_;
    bool running_;
};

/*! Convenience class for nodes that need their own thread.
 *  To use, derive from ThreadedNodeBase and implement threadMain().
 */
class ThreadedNodeBase : public NodeBase {
  public:
    virtual ~ThreadedNodeBase();

    // Starts all output streams + the thread.
    virtual bool start();

    // Stops the thread and all output streams.
    virtual void stop();

    virtual bool isRunning() const;

    bool startThread();

  protected:
    /*! Inheriting classes must implement a thread loop, in the form:
     *  while (!threadMustQuit()) { }
     */
    virtual void threadMain() = 0;

    bool threadMustQuit() const { return thread_must_quit_; }

  private:
    static void threadEntryPoint(void *ptr);
    Thread thread_;
    bool thread_must_quit_;
};

}  // namespace media_graph

#endif  // MEDIAGRAPH_NODE_H
