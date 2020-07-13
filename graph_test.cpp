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

#include "graph.h"
#include "node.h"
#include "stream.h"
#include "stream_reader.h"
#include "types/type_definition.h"

namespace media_graph {

namespace {

class TimestampIncrementerStream : public StreamBase<int> {
  public:
    TimestampIncrementerStream(NodeBase *node) : StreamBase<int>("Timestamp incrementer", node) { sequence_no_ = 0; }
    virtual std::string typeName() const { return "int"; }
    virtual bool tryRead(StreamReader<int>* reader,
                         int* data, Timestamp* timestamp, SequenceId* seq) {
        *timestamp = Timestamp::now();
        *data = ++(*reader->lastReadSequenceIdPtr());
        if (seq) {
            *seq = *data;
        }
        return true;
    }
    virtual bool read(StreamReader<int>* reader,
                      int* data, Timestamp* timestamp, SequenceId *seq) {
        while (!(reader->seekPosition() < Timestamp::now())) {
        }
        return tryRead(reader, data, timestamp, seq);
    }
    virtual bool canRead(SequenceId consumed_until, Timestamp fresher_than) const {
        return consumed_until < sequence_no_ && fresher_than < Timestamp::now();
    }
  private:
    int sequence_no_;
};

class IntProducerNode : public NodeBase {
  public:
    IntProducerNode() : output_stream(this) { }

    virtual int numOutputStream() const { return 1; }
    virtual const NamedStream* constOutputStream(int index) const {
        if (index == 0) return &output_stream;
        return 0;
    }

  private:
    TimestampIncrementerStream output_stream;
};

class IntConsumerNode : public NodeBase {
  public:
    IntConsumerNode() : input("int", this) { }

    virtual int numInputPin() const { return 1; }
    virtual const NamedPin* constInputPin(int index) const {
        if (index == 0) return &input;
        return 0;
    }

    void testSequentialRead(Timestamp after) {
        int value;
        Timestamp timestamp;
        EXPECT_TRUE(input.seek(after));
        EXPECT_TRUE(input.read(&value, &timestamp));
    }

    void testTryRead(Timestamp after) {
        int value;
        Timestamp timestamp;
        EXPECT_TRUE(input.seek(after));
        EXPECT_TRUE(input.tryRead(&value, &timestamp));
    }

    void testReadFrom(Timestamp bound, int num_to_read) {
        EXPECT_TRUE(input.seek(bound));
        Timestamp last_timestamp = bound;
        SequenceId last_sequence_id = -1;
        for (int i = 0; i < num_to_read; ++i) {
            int value;
            Timestamp timestamp;
            SequenceId sequence_id;
            EXPECT_TRUE(input.read(&value, &timestamp, &sequence_id));

            // Make sure the time is monotonic.
            EXPECT_TRUE(!(timestamp < last_timestamp));
            EXPECT_LT(last_sequence_id, sequence_id);
            last_timestamp = timestamp;
            last_sequence_id = sequence_id;
        }
    }

  private:
    StreamReader<int> input;
};

class ThreadedIntProducer : public ThreadedNodeBase {
  public:
    ThreadedIntProducer(Duration limit = Duration())
        : output_stream("out", this), time_limit_(limit) { }

    virtual void threadMain() {
        int sequence_no = 0;
        Timestamp start_time = Timestamp::now();

        while (!threadMustQuit()) {
            // push as fast as we can.
            Timestamp timestamp = Timestamp::now();
            if (time_limit_ != Duration() && (timestamp - start_time) > time_limit_) {
                std::cout << "Time limit reached, exiting producer thread.\n";
                return;
            }
            if (!output_stream.update(timestamp, sequence_no))
                break;
            ++sequence_no;
        }
    }
    virtual int numOutputStream() const { return 1; }
    virtual const NamedStream* constOutputStream(int index) const {
        if (index == 0) return &output_stream;
        return 0;
    }
  private:
    Stream<int> output_stream;
    Duration time_limit_;
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

class JoinAndCheckEquals : public NodeBase {
  public:
    JoinAndCheckEquals() : input_a("a", this), input_b("b", this) { }

    virtual int numInputPin() const { return 2; }
    virtual const NamedPin* constInputPin(int i) const {
        switch(i) {
            case 0: return &input_a;
            case 1: return &input_b;
        }
        return 0;
    }

    void testSyncFromA(int num_iterations) {
        for (int i = 0; i < num_iterations; ++i) {
            int value_a;
            Timestamp timestamp_a;
            EXPECT_TRUE(input_a.read(&value_a, &timestamp_a));
            int value_b;
            Timestamp timestamp_b;
            EXPECT_TRUE(input_b.seek(timestamp_a - Duration::microSeconds(1)));
            EXPECT_TRUE(input_b.read(&value_b, &timestamp_b));
            EXPECT_EQ(0, (timestamp_a - timestamp_b).milliSeconds());
            EXPECT_EQ(value_a, value_b);
        }
    }

  private:
    StreamReader<int> input_a;
    StreamReader<int> input_b;
};


}  // namespace

// producer -> consumer
TEST(GraphTest, NoThread) {
    Graph graph;

    auto producer = graph.newNode<IntProducerNode>("producer");
    auto consumer = graph.newNode<IntConsumerNode>("consumer");
    EXPECT_TRUE(graph.connect("producer", "Timestamp incrementer", "consumer", "int"));

    // Make sure "connect" does some verifications.
    EXPECT_FALSE(graph.connect("invalid node", "Timestamp incrementer", "consumer", "int"));
    EXPECT_FALSE(graph.connect("producer", "invalid stream", "consumer", "int"));
    EXPECT_FALSE(graph.connect("producer", "Timestamp incrementer", "invalid node", "int"));
    EXPECT_FALSE(graph.connect("producer", "Timestamp incrementer", "consumer", "invalid pin"));

    EXPECT_TRUE(graph.start());

    consumer->testSequentialRead(Timestamp::now() + Duration::milliSeconds(10));
    consumer->testTryRead(Timestamp::now());
}

// producer -> filter -> consumer
TEST(GraphTest, SimpleThreaded) {
    Graph graph;
    auto producer = graph.newNode<ThreadedIntProducer>("producer");
    auto filter = graph.newNode<ThreadedPassThrough>("filter");
    auto consumer = graph.newNode<IntConsumerNode>("consumer");

    EXPECT_TRUE(graph.connect(producer, "out", filter, "in"));
    EXPECT_TRUE(graph.connect(filter, "out", consumer, "int"));
    EXPECT_TRUE(graph.start());

    consumer->testReadFrom(Timestamp::now(), 100);

    graph.stop();
}

TEST(GraphTest, HotPlug) {
    Graph graph;

    // The graph is started first.
    graph.start();

    // We add nodes.
    auto producer = graph.newNode<ThreadedIntProducer>("producer");
    auto filter = graph.newNode<ThreadedPassThrough>("filter");
    auto consumer = graph.newNode<IntConsumerNode>("consumer");

    EXPECT_TRUE(graph.connect(producer, "out", filter, "in"));
    EXPECT_TRUE(graph.connect(filter, "out", consumer, "int"));

    // The nodes have to be started.
    EXPECT_TRUE(producer->start());
    EXPECT_TRUE(filter->start());
    EXPECT_TRUE(consumer->start());

    EXPECT_TRUE(producer->isRunning());
    EXPECT_TRUE(filter->isRunning());
    EXPECT_TRUE(consumer->isRunning());

    // Data should be flowing.
    consumer->testReadFrom(Timestamp::now(), 10);

    // Waow, that's rude.
    graph.removeNode("filter");
    filter.reset();

    EXPECT_FALSE(consumer->isRunning());

    EXPECT_TRUE(graph.connect(producer, "out", consumer, "int"));
    EXPECT_TRUE(consumer->start());

    EXPECT_TRUE(producer->isRunning());
    EXPECT_TRUE(consumer->isRunning());

    consumer->testReadFrom(Timestamp::now(), 10);

    graph.stop();
}

/*
              /--> a --\
             /          \
 producer ->-            ----> consumer
             \          / 
              \--> b --/

*/

TEST(GraphTest, JoinSync) {
    Graph graph;
    auto producer = graph.newNode<ThreadedIntProducer>("producer");
    auto filter_a = graph.newNode<ThreadedPassThrough>("a");
    auto filter_b = graph.newNode<ThreadedPassThrough>("b");
    auto consumer = graph.newNode<JoinAndCheckEquals>("consumer");

    EXPECT_EQ(0, producer->getOutputStreamByName("out")->numReaders());
    EXPECT_TRUE(graph.connect(producer, "out", filter_a, "in"));
    EXPECT_EQ(1, producer->getOutputStreamByName("out")->numReaders());
    EXPECT_TRUE(graph.connect("producer", "out", "b", "in"));
    EXPECT_EQ(2, producer->getOutputStreamByName("out")->numReaders());

    EXPECT_TRUE(graph.connect("a", "out", "consumer", "a"));
    EXPECT_TRUE(graph.connect("b", "out", "consumer", "b"));


    EXPECT_TRUE(graph.start());

    consumer->testSyncFromA(10);
    graph.stop();
}
    
class ThreadedIntConsumer : public ThreadedNodeBase {
  public:
    ThreadedIntConsumer() : input_("in", this) { }

    virtual int numInputPin() const { return 1; }
    virtual const NamedPin* constInputPin(int i) const {
        switch(i) {
            case 0: return &input_;
        }
        return 0;
    }

    void threadMain() {
        while (!threadMustQuit()) {
            int value;
            Timestamp timestamp;

            // Here, we "forget" to check the return value of input_, on purpose.
            // That is to make sure threadMustQuit() returns true when at least one input
            // becomes invalid.
            input_.read(&value, &timestamp);
        }
    }
  private:
    StreamReader<int> input_;
};

TEST(GraphTest, shouldNoticeWhenStopped) {
    Graph graph;
    auto producer = graph.newNode<ThreadedIntProducer>("producer", Duration::milliSeconds(50));
    auto consumer = graph.newNode<ThreadedIntConsumer>("consumer");

    EXPECT_TRUE(graph.connect(producer, "out", consumer, "in"));
    EXPECT_TRUE(graph.start());

    EXPECT_TRUE(graph.isStarted());
    graph.waitUntilStopped();
    EXPECT_FALSE(graph.isStarted());
}

}  // namespace media_graph
