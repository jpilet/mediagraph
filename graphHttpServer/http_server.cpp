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
#include "http_server.h"

#include <string.h>
#include <iostream>

//#include <base/string.h>

#include "civetweb/include/CivetServer.h"


class HttpServerCivetHandler: public CivetHandler {
  public:
    HttpServerCivetHandler(HttpServer* server) : server_(server) { }

    bool handleGet(CivetServer *server, struct mg_connection *conn) {
        const struct mg_request_info *request_info = mg_get_request_info(conn);
          //printf("New request: uri: %s query_string: %s\n",
          //       request_info->uri, request_info->query_string);
          std::unique_ptr<HttpReply> reply(new HttpReply(conn));
          if ( server_->onNewRequest(std::move(reply)) ) {
              return true;
          }
          return false;
    }
  private:
    HttpServer* server_;
};


HttpServer::HttpServer(int port, std::string publicDirectory) {
    char port_as_string[16];
    sprintf(port_as_string, "%d", port);
    const char *options[] = {
        "listening_ports", port_as_string,
        "enable_keep_alive", "yes",
        "document_root" , publicDirectory.c_str(),
        NULL};
    civet_server_.reset(new CivetServer(options));
    handler_.reset(new HttpServerCivetHandler(this));
    civet_server_->addHandler("", *handler_);
}

HttpServer::~HttpServer() {
}

void HttpReply::send() {
    mg_printf(conn_,
            "HTTP/1.1 %s\r\n"
            "Connection: keep-alive\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"        // Always set Content-Length
            "\r\n"
            "%s",
            status.c_str(),
            content_type.c_str(),
            text.size(),
            text.c_str());
}

void HttpReply::handleJsonp() {
    const struct mg_request_info *request_info = mg_get_request_info(conn_);
    const char *qs = request_info->query_string;

    char cb[64];
    mg_get_var(qs, strlen(qs == NULL ? "" : qs), "callback", cb, sizeof(cb));

    if (cb[0] != '\0') {
        text = std::string(cb) + '(' + text + ')';
    }
    setAjaxContent();
}

std::string HttpReply::getQSvar(std::string varName) {
    const struct mg_request_info *request_info = mg_get_request_info(conn_);
    const char *qs = request_info->query_string;
    int bufferSize = 256;
    std::vector<char> bufferVar(bufferSize);
    int ret = mg_get_var(qs, strlen(qs == NULL ? "" : qs), varName.c_str(), &bufferVar[0], 256);
    while (ret == -2) {
         // the recieved buffer is too small
        bufferSize *= 2; //double it
        bufferVar.reserve( bufferSize );
        ret = mg_get_var(qs, strlen(qs == NULL ? "" : qs), varName.c_str(), &bufferVar[0], 256);
        if (bufferSize > 10000000) {
          std::cerr << "getQSvar: Variable too long > 10 Mo : "
            << varName << std::endl;
        }
    }
    if (ret == -1) {
        std::cerr << "getQSvar: Variable not found " << varName << "\n";
        return std::string();
    }
    return std::string(&bufferVar[0]);
}

std::string HttpReply::getUri() {
    return mg_get_request_info(conn_)->local_uri;
}

