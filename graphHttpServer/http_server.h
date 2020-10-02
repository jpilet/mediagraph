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
#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <functional>
#include <map>
#include <memory>
#include <string>

class CivetServer;
class HttpServerCivetHandler;

extern "C" struct mg_connection;

/*! This structure contains the reply to the http request
 *  for the given connection
 */
class HttpReply {
public:
    HttpReply(mg_connection* conn) : conn_(conn) {
        setOK();
        setTextContent();
    }

    void setOK() { status = "200 OK"; }

    void setNotFound() { status = "404 Not Found"; }

    void setAjaxContent() { content_type = "application/x-javascript"; }

    void setTextContent() { content_type = "text/plain"; }

    void send();

    void handleJsonp();

    std::string getUri();

    std::string getQSvar(std::string varName);

    std::string text;
    std::string status;
    std::string content_type;

private:
    mg_connection* conn_;
};

/*! This class opens a web server using mongoose.
 * The constructors starts the server, the destructor stops it.
 * By default, the server serves files in the current directory.
 * To change this behavior, the "publicDirectory" argument of the
 * constructor tells where to serve the files from. To add dynamic
 * pages, overload onNewRequest().
 */
class HttpServer {
public:
    enum Method { GET, POST, HEAD, PUT, DELETE, OPTIONS, PATCH };

    HttpServer(int port, const std::string& publicDirectory = ".");
    virtual ~HttpServer();

    void setHandler(Method method, const std::string& uri,
                    std::function<bool(std::unique_ptr<HttpReply>)> cb);

private:
    friend class HttpServerCivetHandler;

    // Copy construction is forbidden
    HttpServer(const HttpServer&) = delete;

    std::unique_ptr<CivetServer> civet_server_;
    std::map<std::string, std::shared_ptr<HttpServerCivetHandler>> handlers_;
};

#endif  // HTTP_SERVER_H
