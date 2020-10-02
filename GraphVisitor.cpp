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
#include "GraphVisitor.h"

namespace media_graph {
void GraphVisitor::visit(Graph* graph) {
    for (int i = 0; i < graph->numProperty(); i++) { onProperty(0, 0, 0, graph->property(i)); }

    for (int i = 0; i < graph->numNodes(); i++) {
        std::shared_ptr<NodeBase> node = graph->node(i);
        onNode(node);

        for (int j = 0; j < node->numProperty(); j++) { onProperty(node, 0, 0, node->property(j)); }

        for (int j = 0; j < node->numOutputStream(); j++) {
            NamedStream* stream = node->outputStream(j);
            onStream(node, stream);
            for (int k = 0; k < stream->numProperty(); k++) {
                onProperty(node, stream, 0, stream->property(k));
            }
        }

        for (int j = 0; j < node->numInputPin(); j++) {
            NamedPin* pin = node->inputPin(j);
            onPin(node, pin);
            for (int k = 0; k < pin->numProperty(); k++) {
                onProperty(node, 0, pin, pin->property(k));
            }
        }
    }
}

}  // namespace media_graph
