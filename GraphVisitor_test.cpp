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

#include "GraphVisitor.h"

#include "graph.h"
#include "node.h"
#include "stream.h"
#include "stream_reader.h"


namespace media_graph {

namespace {

class NodeWithProperty : public NodeBase {
public:
    NodeWithProperty (const std::string& wanted_name, Graph* graph)
        : NodeBase(wanted_name, graph), A("int A", 1) { }

    virtual int numProperty() const { return 1 + PropertyList::numProperty(); }
    virtual NamedProperty* property(int id) {
        switch (id) {
            case 0: return &A;
        }
        return PropertyList::property(id - 1);
    }

    void setA(int val) { A = val;}
private:
    Property<int> A;
};


class GraphAnalyser: GraphVisitor {

public:
    int getNodeCount(Graph * graph) {
        nodeCount = 0;
        visit(graph);
        return nodeCount;
    }

protected:
    virtual void onNode(NodeBase * node) {
        EXPECT_EQ("node prop int",node->name());
        nodeCount++;
    }

    virtual void onStream(NodeBase * node, NamedStream * stream) {
        EXPECT_TRUE(false) << "no stream in graph-should not pass here";
    }
    
    virtual void onPin(NodeBase * node, NamedPin * pin) {
        EXPECT_TRUE(false) << "no pin in graph-should not pass here";
    }
    
    virtual void onProperty(NodeBase * node, NamedStream * stream, NamedPin * pin, NamedProperty * prop) {
        if (node==0) {
            //test on graph property
            EXPECT_EQ("started",prop->name());
        } else {
             EXPECT_EQ("node prop int",node->name());
             //test property name
             if ( (stream == 0) && (pin == 0) ) {
                 EXPECT_EQ("int A",prop->name());
             }
        }
    }  

private:
    int nodeCount;
};

TEST(GraphVisitor, parseGraph) {

    Graph graph;
    NodeWithProperty * node  = new NodeWithProperty("node prop int", &graph);

    GraphAnalyser grfAnal;
   
    EXPECT_EQ(1,grfAnal.getNodeCount(&graph));

}

} //namespace anonymous

} //namespace media_graph

