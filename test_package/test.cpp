#include <mediagraph/graph.h>
#include <mediagraph/stream.h>
#include <mediagraph/stream_reader.h>

#include <iostream>

struct MyData
{
    std::string str;
    int64_t seq;
};

namespace media_graph
{
namespace
{
    template <>
    std::string typeName<MyData>()
    {
        return "MyData";
    }

}  // namespace
}  // namespace media_graph

using namespace media_graph;

class DataStream : public Stream<MyData>
{
public:
    static const int maxQueueSize = 100;
    DataStream(NodeBase *node)
        : Stream<MyData>("DataStream", node, DROP_READ_BY_ALL_READERS, maxQueueSize)
    {
    }

    virtual std::string typeName() const { return "MyData"; }
};

class DataProducerNode : public ThreadedNodeBase
{
public:
    DataProducerNode() : m_dataStream(this) {}

    virtual int numOutputStream() const { return 1; }
    virtual const NamedStream *constOutputStream(int index) const
    {
        if (index == 0) return &m_dataStream;
        return 0;
    }

    virtual void threadMain()
    {
        MyData data;
        data.seq = 0;
        data.str = "something in the way..";

        for (int i = 0; i < 100; ++i)
        {  // send 100 values
            if (m_dataStream.canUpdate()) { m_dataStream.update(Timestamp::now(), data); }

            ++data.seq;
        }
    }

private:
    DataStream m_dataStream;
};

class DataConsumerNode : public ThreadedNodeBase
{
public:
    DataConsumerNode() : input("input_pin_name", this) {}

    virtual void threadMain()
    {
        int64_t lastSeq = -1;

        while (!threadMustQuit())
        {
            MyData data;
            Timestamp ts;
            SequenceId seqId;

            // read will return false when the graph is stopped
            if (!input.read(&data, &ts, &seqId))
            {
                if (lastSeq != 99) { throw std::runtime_error("lastSeq should be 99 by now!"); }
                return;
            }

            if (lastSeq + 1 != data.seq)
            {
                throw std::runtime_error("Sequence out of order!");  // will crash app and fail test
            }

            lastSeq = data.seq;
        }
    }

    virtual int numInputPin() const { return 1; }
    virtual const NamedPin *constInputPin(int index) const
    {
        if (index == 0) return &input;
        return 0;
    }

private:
    StreamReader<MyData> input;
};

#define EXPECT_TRUE(x, y)                                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(x)) { throw std::runtime_error("Error expect true " + std::string(y)); } \
    } while (0)

int main()
{
    Graph graph;

    auto producer = graph.newNode<DataProducerNode>("producer");
    auto consumer = graph.newNode<DataConsumerNode>("consumer");

    EXPECT_TRUE(graph.connect("producer", "DataStream", "consumer", "input_pin_name"), "connect");

    EXPECT_TRUE(graph.start(), "start");

    std::cout << "Waiting for producer to finish..." << std::endl;
    while (producer->isRunning()) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }

    std::cout << "Stopping graph..." << std::endl;
    graph.stop();
    std::cout << "Done." << std::endl;

    return 0;
}
