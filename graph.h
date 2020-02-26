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
// Julien.Pilet@gmail.com, 2012, 2020.
#ifndef MEDIAGRAPH_GRAPH_H
#define MEDIAGRAPH_GRAPH_H

#include "node.h"
#include "thread_primitives.h"
#include "property.h"

#include <string>
#include <map>
#include <memory>
#include <sstream>

namespace media_graph {

/*! Represent a graph of media producers, filters, and consumers.
 *  
 *  In the graph, nodes can produce and consume data. A timestamp is associated
 *  with each data. Each node might or might not have its own thread. The
 *  output of a node can go to any number of other nodes. Graph building is
 *  done at runtime.
 *
 *  To build a Graph, construct some nodes, and call connect() for all
 *  edges. Once your graph is built, call start(). If start() succeeded, data
 *  should start flowing.
 *
 *  Example:
 *
 *  \code
 *  Graph graph;
 *  
 *  ProducerNode *producer = new ProducerNode("producer", &graph);
 *  ConsumerNode *consumer = new ConsumerNode("consumer", &graph);
 *
 *  if (!graph.connect(producer, "out", consumer, "in")
 *      || !graph.start()) {
 *    // something went wrong.  
 *  } else {
 *    // graph is running.
 *  }
 *  \endcode
 */
class Graph : public PropertyList {
  public:
    Graph();
    ~Graph() { clear(); }

    /// Construct a new node, add it to the graph, and returns a shared_ptr.
    template<typename T, typename ... Args>
    std::shared_ptr<T> newNode(const std::string& wanted_name, Args && ... args);

    /*! Adds a node to the graph.
     *
     *  Once added, a node can be retrieved using its name with getNodeByName.
     *
     *  If the graph is running, it will be first stopped. The user will have
     *  to restart it.
     *
     *  Returns true on success. Returns false if a node <name> already exists
     *  in the graph.
     */
    bool addNode(const std::string& name, std::shared_ptr<NodeBase> node);

    /*! Removes a node from the node list. The easiest way to remove a node
     *  in practice is to delete it directly, the destructor will call
     *  removeNode().
     */
    void removeNode(const std::string& name);

    /*! Returns the node that was previously added with the given name. If no
     *  matching node is found, returns 0.
     */
    std::shared_ptr<NodeBase> getNodeByName(const std::string& name);

    /*! Adds an edge to the graph.
     *  Connects the output stream called <streamName> of source node <source>
     *  to the pin called <pinName> on the node <dest>.
     *
     *  Behavior undefined if called after start(). Call stop() before
     *  modifying the graph.
     *
     *  returns true on success.
     */
    bool connect(NamedStream* stream, NamedPin* pin);

    bool connect(std::shared_ptr<NodeBase> source, const std::string& streamName,
                 std::shared_ptr<NodeBase> dest, const std::string& pinName);

    //! Adds an edge to the graph.
    bool connect(const std::string& source, const std::string& streamName,
                 const std::string& dest, const std::string& pinName);

    /*! Start the graph: calls start() on every node.
     *  Returns true if all nodes started properly. If a node refuses to start,
     *  all already started nodes are stopped and start() returns false.
     */
    bool start();

    //! Tells if the graph has been successfully started.
    bool isStarted() const { return started_; }

    //! Stops the graph. Does nothing if the graph is already stopped.
    void stop();

    /*! Remove and delete all nodes in the graph. Stops the graph first if
     *  necessary.
     */
    void clear();

    int numNodes() const { return nodes_.size(); }

    std::shared_ptr<NodeBase> node(int num) const;

  private:
    // Stop the graph, assumes mutex_ is already aquired.
    void lockedStop();
    std::shared_ptr<NodeBase> lockedGetNodeByName(const std::string& name);

    Graph(const Graph&) { }  // copy constructor is forbidden.


    std::map<std::string, std::shared_ptr<NodeBase>> nodes_;
    bool started_;

    // Protects nodes_ against node addition and removal from multiple threads.
    Mutex mutex_;
};


template<typename T, typename ... Args>
std::shared_ptr<T> Graph::newNode(const std::string& wanted_name, Args && ... args) {
    std::shared_ptr<T> ptr = std::make_shared<T>(args...);

    std::string try_name = wanted_name;
    for (int i = 0; !addNode(try_name, ptr); ++i) {
        std::ostringstream oss;
        oss << wanted_name << i;
        try_name = oss.str();
    }
    return ptr;
}

}  // namespace media_graph

#endif  // MEDIAGRAPH_GRAPH_H
