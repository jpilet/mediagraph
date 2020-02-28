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
#ifndef GRAPH_HTTP_SERVER_H
#define GRAPH_HTTP_SERVER_H

#include "http_server.h"

namespace media_graph {

class Graph;

/*! This class opens a web server that gives access to a media_graph::Graph object.
 * The constructors starts the server, the destructor stops it.
 *
 * The executable expects to find the content of the "html" directory
 * in current directory.
 */
class GraphHttpServer : HttpServer {
  public:
    //! Starts the http server. "graph" has to remain valid during the life
    //! of the constructed object.
    GraphHttpServer(Graph *graph, int port);
    virtual ~GraphHttpServer() {}

  private:

      virtual  bool onNewRequest(HttpReply * reply);

    Graph* graph_;
};

}  // namespace media_graph

#endif  // GRAPH_HTTP_SERVER_H
