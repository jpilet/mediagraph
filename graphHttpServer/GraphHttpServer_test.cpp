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

#include "../timestamp.h"
#include "../graph.h"
#include "../graphHttpServer/GraphHttpServer.h"
#include "../node.h"
#include "../stream.h"
#include "../stream_reader.h"
#include "../types/type_definition.h"

namespace media_graph {

class ThreadedIntProducer : public ThreadedNodeBase {
  public:
    ThreadedIntProducer() : output_stream("out", this) { }
    virtual void threadMain() {
        Timestamp timestamp;
        int sequence = 0;
        while (!threadMustQuit()) {
            // push as fast as we can.
            if (!output_stream.update(timestamp, sequence))
                break;
            sequence++;
        }
    }
    virtual int numOutputStream() const { return 1; }
    virtual const NamedStream* constOutputStream(int index) const {
        if (index == 0) return &output_stream;
        return 0;
    }
  private:
    Stream<int> output_stream;
};

class ThreadedPassThrough : public ThreadedNodeBase {
  public:
    ThreadedPassThrough()
        : output_stream("out", this), input_stream("in", this) { }

    virtual void threadMain() {
        while (!threadMustQuit()) {
            int data;
            Timestamp timestamp;
            if (!input_stream.read(&data, &timestamp)) {
                break;
            }
            if (!output_stream.update(timestamp, data)) {
                break;
            }
        }
    }

    virtual int numInputPin() const { return 1; }
    virtual const NamedPin* constInputPin(int index) const {
        return (index == 0 ? &input_stream : 0);
    }
    virtual int numOutputStream() const { return 1; }
    virtual const NamedStream* constOutputStream(int index) const {
        if (index == 0) return &output_stream;
        return 0;
    }

  private:
    Stream<int> output_stream;
    StreamReader<int> input_stream;
};

void ConstructGraph(Graph *graph) {
    auto producer = graph->newNode<ThreadedIntProducer>("producer");
    auto passThrough = graph->newNode<ThreadedPassThrough>("passthrough");
    graph->connect(producer->outputStream(0), passThrough->inputPin(0));
    graph->start();
}

}  // namespace media_graph

int main(int argc, char **argv) {
    media_graph::Graph graph;
    media_graph::ConstructGraph(&graph);

    media_graph::GraphHttpServer server(&graph, 1212);

    while(1) {
        Duration::milliSeconds(10).sleep();
    }

    return 0;
}
