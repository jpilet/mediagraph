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
#include "GraphHttpServer.h"

#include <sstream>
#include <iomanip>
#include <string.h>
#include "SplitString.h"

//#include <base/string.h>
#include "../graph.h"
#include "../stream.h"
#include "../stream_reader.h"
#include "civetweb/include/civetweb.h"

namespace media_graph {

namespace {

// see https://stackoverflow.com/questions/7724448/simple-json-string-escape-for-c
std::string EscapeJson(const std::string &s) {
    std::ostringstream o;
    o << '"';
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        switch (*c) {
        case '"': o << "\\\""; break;
        case '\\': o << "\\\\"; break;
        case '\b': o << "\\b"; break;
        case '\f': o << "\\f"; break;
        case '\n': o << "\\n"; break;
        case '\r': o << "\\r"; break;
        case '\t': o << "\\t"; break;
        default:
            if ('\x00' <= *c && *c <= '\x1f') {
                o << "\\u"
                  << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
            } else {
                o << *c;
            }
        }
    }
    o << '"';
    return o.str();
}

class ToJsonValue : public TypeConstVisitor {
  public:
    virtual bool process(const int &value) {
        result_ = std::to_string(value);
        return true;
    }
    virtual bool process(const int64_t &value) {
        result_ = std::to_string(value);
        return true;
    }
    virtual bool process(const bool &value) {
        result_ = (value ? "true" : "false");
        return true;
    }
    virtual bool process(const float &value) {
        result_ = std::to_string(value);
        return true;
    }
    virtual bool process(const double &value) {
        result_ = std::to_string(value);
        return true;
    }
    virtual bool process(const std::string &value) {
        result_ = EscapeJson(value);
        return true;
    }

    std::string json() const { return result_; }
  private:
    std::string result_;
};

void ListNodes(Graph *graph, HttpReply *reply) {
    reply->text += '[';
    for (int i = 0; i < graph->numNodes(); ++i) {
        reply->text += '"' + graph->node(i)->name() + '"';
        if (i + 1 < graph->numNodes()) {
            reply->text += ",";
        }
    }
    reply->text += ']';
}

void ListProperties(PropertyList *list, HttpReply *reply) {
    if (list == 0) {
        reply->setNotFound();
        return;
    }

    std::ostringstream ss;
    ss << "[";
    for (int i = 0; i < list->numProperty(); ++i) {
        const NamedProperty *property = list->property(i);
        ToJsonValue converter;
        property->apply(&converter);
        ss << "{name:" <<  EscapeJson(property->name())
          << ",type:" << EscapeJson(property->typeName())
          << ",value:" << converter.json() << "}";

        if (i + 1 < list->numProperty()) {
            ss << ",";
        }
    }
    ss << "]";

    reply->text += ss.str();
}

void ServeNode(NodeBase *node, HttpReply *reply) {
    if (!node) {
        reply->setNotFound();
        return;
    }

    std::ostringstream ss;
    ss << "{name:" << EscapeJson(node->name()) << ",output:[";

    for (int i = 0; i < node->numOutputStream(); ++i) {
        NamedStream *stream = node->outputStream(i);
        ss << "{name:" << EscapeJson(stream->streamName())
          << ",type:" << EscapeJson(stream->typeName()) << "}";
        if (i + 1 < node->numOutputStream()) {
            ss << ',';
        }
    }
    ss << "],input:[";

    for (int i = 0; i < node->numInputPin(); ++i) {
        NamedPin *pin = node->inputPin(i);
        ss << "{name:" << EscapeJson(pin->name())
          << ",type:" << EscapeJson(pin->typeName());
        if (pin->isConnected()) {
            ss << ",connection:{node:"
              << EscapeJson(pin->connectedStream()->node()->name())
              << ",stream:" << EscapeJson(pin->connectedStream()->streamName())
              << "}";
        }
        ss << "}";
        if (i + 1 < node->numInputPin()) {
            ss << ',';
        }
    }
    ss << "]}";
    reply->text += ss.str();
}

// Handle URI with prefix /node/nodeName, assuming nodeName is valid.
void ServeValidNodeDir(const std::vector<std::string>& directories,
                       NodeBase *node, HttpReply *reply) {

    if (directories.size() == 2) {
        // /node/<node name>, we list pins and streams.
        ServeNode(node, reply);
    } else {
        if (directories[2] == "props") {
            // /node/<node name>/props, we list node properties.
            ListProperties(node, reply);
        } else if (directories[2] == "stream") {
            if (directories.size() >= 4) {
                // /node/<node name>/stream/<stream name>
                // we list stream properties
                ListProperties(node->getOutputStreamByName(directories[3]), reply);
            } else {
                reply->text = "No stream name given\r\n";
                reply->setNotFound();
            }
        } else if (directories[2] == "pin") {
            if (directories.size() >= 4) {
                // /node/<node name>/pin/<pin name>
                // we list pin properties.
                ListProperties(node->getInputPinByName(directories[3]), reply);
            } else {
                reply->text = "No pin name given\r\n";
                reply->setNotFound();
            }
        } else {
            reply->setNotFound();
        }
    }
}

// Handle URI with prefix: /node
void ServeNodeDir(const std::vector<std::string>& directories,
                  Graph *graph, HttpReply *reply) {

    if (directories.size() >= 2) {
        // We have /node/<node name>/...
        std::shared_ptr<NodeBase> node = graph->getNodeByName(directories[1]);
        if (node) {
            ServeValidNodeDir(directories, node.get(), reply);
        } else {
            reply->text += "Node not found.\r\n";
            reply->setNotFound();
        }
    } else {
        reply->text = "Node not specified\r\n";
        reply->setNotFound();
    }
}

}  // namespace

GraphHttpServer::GraphHttpServer(Graph *graph, int port) : HttpServer(port), graph_(graph) {

}


bool GraphHttpServer::onNewRequest(HttpReply * reply) {
   
    std::string uri = reply->getUri();
    std::vector<std::string> directories = SplitString(uri, "/");

    if (directories.size() == 0) {
        // Let mongoose serve the file.
        return false;
    } else {
        if (directories[0] == "props") {
            ListProperties(graph_, reply);
        } else if (directories[0] == "node") {
            ServeNodeDir(directories, graph_, reply);
        } else if (directories[0] == "nodeList") {
            ListNodes(graph_, reply);
        } else if (directories[0] == "html") {
            // Let mongoose serve the file.
            delete(reply);
            return false;
        } else {
            reply->setNotFound();
        }
    }

    if (reply->status[0] == '2') {
        reply->setAjaxContent();
        reply->handleJsonp();
    }

    reply->send();
    delete(reply);

    // Mark as processed
    return true;
}

}  // namspace media_graph
