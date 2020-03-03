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
/*
Here's a typical story of a browser loading a page showing a media graph.

1. the browser loads the page (index.html, main.js, style.css)
2. the browser calls $(document).ready() defined at the bottom of this file
3. this script asks the server for information about the graph and its
   properties
4. when the server replies, the page gets populated with data.

showNodeList, showProperties, and showNode are called when a reply is received
from the server.

getNodeList, getProperties, and getNode send queries to the server.

*/

var graphClient = {
  // Backend URL, string.
  // 'http://backend.address.com' or '' if backend is the same as frontend
  backendUrl: '',
  selectedNode: '',
  selectedPin: '',
  selectedStream: '',
};

graphClient.normalizeText = function(text) {
  return text.replace('<', '&lt;').replace('>', '&gt;');
};

// showNodeList() gets called when the server told us about the graph.
// For each node in the reply, the function adds a "span" tag to the element
// with id=nodeList.
graphClient.showNodeList = function(data) {
  if (data === undefined) {
    return;
  }
  
  $('#nodeList').html('');

  $.each(data, function(index, entry) {
    $('<span>')
      .addClass('node-entry')
      .click(graphClient.selectNode)
      .html(entry)
      .appendTo('#nodeList');
  });
};

// Populate the "id=props" element with properties given by the server.
graphClient.showProperties = function(data) {
  if (data === undefined) {
    return;
  }
  
  $('#props').html('');

  $.each(data, function(index, entry) {
    var row = $('<div>').addClass('property-row').appendTo('#props');
    $('<span>').addClass('property-name').html(entry.name).appendTo(row);
    $('<span>').addClass('property-type').html(entry.type).appendTo(row);
    $('<span>').addClass('property-value').html(String(entry.value)).appendTo(row);
  });
};

// Populate the "id=node" element with node data returned by the server.
// The data object is expected to have the following members:
//    - name: the node name;
//    - output: a list of output streams;
//    - input: a list of input pins;
graphClient.showNode = function(data) {
  if (data === undefined) {
    return;
  }
  
  $('#node').html('');

  $('<div>').addClass('node-header').html(data.name).appendTo('#node');

  $.each(data.output, function(index, entry) {
    var row = $('<div>').addClass('output-stream-row').appendTo('#node');
    $('<span>')
        .addClass('output-stream-name')
        .html(entry.name)
        .click(graphClient.selectStream)
        .appendTo(row);
    $('<span>').addClass('output-stream-type').html(entry.type).appendTo(row);
  });

  $.each(data.input, function(index, entry) {
    var row = $('<div>').addClass('input-pin-row').appendTo('#node');
    $('<span>')
          .addClass('input-pin-name')
          .html(entry.name)
          .click(graphClient.selectPin)
          .appendTo(row);
    $('<span>').addClass('input-pin-type').html(entry.type).appendTo(row);
    if (entry.connection !== undefined) {
      $('<span>').addClass('input-pin-connection-node')
          .html(entry.connection.node)
          .appendTo(row);
      $('<span>').addClass('input-pin-connection-stream')
          .html(entry.connection.stream)
          .appendTo(row);
    }
  });
};

graphClient.getNodeList = function() {
  $.ajax({
    dataType: 'jsonp',
    url: graphClient.backendUrl + '/nodeList',
    data: { },
    success: graphClient.showNodeList,
    error: function(ev) {
      $('#motd').html('Error getting node list');
    },
  });
};

graphClient.getProperties = function() {
  var node = graphClient.selectedNode;
  var path = '';
  if (node != '') {
    path = '/node/' + node;
  }

  var pin = graphClient.selectedPin;
  if (pin != '') {
    path += '/pin/' + pin;
  } else {
    var stream = graphClient.selectedStream;
    if (stream != '') {
      path += '/stream/' + stream;
    }
  }

  $.ajax({
    dataType: 'jsonp',
    url: graphClient.backendUrl + path + '/props',
    data: { },
    success: graphClient.showProperties,
    error: function(ev) {
      $('#motd').html('Error getting properties for '
                      + path + ':' + ev.statusText);
    },
  });
};

graphClient.getNode = function() {
  var node = graphClient.selectedNode;
  if (node == '') {
    $('#node').html('');
  } else {
    $.ajax({
      dataType: 'jsonp',
      url: graphClient.backendUrl + '/node/' + node,
      data: { },
      success: graphClient.showNode,
      error: function(ev) {
      $('#motd').html('Error getting node ' + node);
      },
    });
  }
};

graphClient.selectStream = function(ev) {
  graphClient.selectedPin = '';
  graphClient.selectedStream = $(this).text();
  graphClient.getProperties();
}

graphClient.selectPin = function(ev) {
  graphClient.selectedStream = '';
  graphClient.selectedPin = $(this).text();
  graphClient.getProperties();
}

graphClient.selectNode = function(ev) {
  graphClient.selectedPin = '';
  graphClient.selectedStream = '';
  graphClient.selectedNode = '';
  if ($(this).hasClass('node-entry-selected')) {
    $(this).removeClass('node-entry-selected');
  } else {
    $('.node-entry').removeClass('node-entry-selected');  // Deselect menu buttons
    $(this).addClass('node-entry-selected');
    graphClient.selectedNode = $(this).text();
  }
  graphClient.getProperties();
  graphClient.getNode();
}

$(document).ready(function() {
  graphClient.getNodeList();
  graphClient.getProperties();
});

// vim:ts=2:sw=2:et
