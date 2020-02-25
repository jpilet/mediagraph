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
/*!
 * \author Julien Pilet
 * \date 2012
 */

#ifndef _STREAM_H
#define _STREAM_H

#include "thread_primitives.h"
#include "timestamp.h"
#include "property.h"

#include <assert.h>
#include <deque>
#include <string>
#include <vector>

namespace media_graph {

class NodeBase;

typedef int64_t SequenceId;

// Forward declaration to "friend" StreamReader.
template <typename T> class StreamReader;
class NamedPin;

class NamedStream : public PropertyList {
  public:
    NamedStream(const std::string& name, NodeBase* node) : name_(name), node_(node) { }

    virtual ~NamedStream() { disconnectReaders(); }
    virtual std::string typeName() const = 0;
    const std::string& streamName() const { return name_; };
    virtual void open() { }
    virtual void close() { }

    virtual void registerReader(NamedPin* reader);
    virtual bool unregisterReader(NamedPin* reader);
    bool isReaderRegistered(NamedPin* reader) const;

    int numReaders() const { return readers_.size(); }
    NamedPin* reader(int index) const { return readers_[index]; }

    void disconnectReaders();
    NodeBase *node() const { return node_; }

  protected:
    mutable Mutex mutex_;
    void lock() const { mutex_.lock(); }
    void unlock() const { mutex_.unlock(); }
    virtual void markReadAfter(SequenceId seq) { };

  private:
    std::vector<NamedPin*> readers_;
    std::string name_;
    NodeBase* node_;
};

namespace {
template <typename T>
std::string typeName();
}

/*! Read interface for streams. Typically, nodes in the graph keep pointers to
 *  StreamBase<T> objects, through a StreamReader<T>.
 */
template <typename T>
class StreamBase : public NamedStream {
  public:
    StreamBase(const std::string& name, NodeBase* node) : NamedStream(name, node) { }
    virtual ~StreamBase() {}

  protected:

    /*! Reads an entry, potentially blocking if nothing is available yet.
     *  This method is intended to be called only from a StreamReader<T>
     *  object.
     * \param fresher_than the called wants data with a timestamp greater
     *                     than this. A negative value means: anything.
     * \param data Where to copy the data to.
     * \param timestamp the timestamp corresponding to <data>.
     * \return True on success, false on stream error. If true, data and
     *         timestmap have been written to.
     */
    virtual bool read(StreamReader<T>* reader,
                      T* data, Timestamp* timestamp, SequenceId *seq) = 0;

    /*! A non-blocking read. Same as read(), except that if no data is
     * available, the function returns false immediately instead of waiting
     * for data to arrive.
     * This method is intended to be called only from a StreamReader<T>
     * object.
     */
    virtual bool tryRead(StreamReader<T>* reader,
                         T* data, Timestamp* timestamp, SequenceId* seq) = 0;

    virtual bool canRead(SequenceId consumed_until,
                         Timestamp fresher_than) const = 0;

    // StreamReader is the only one allowed to read data.
    friend class StreamReader<T>;
};

enum StreamDropPolicy {
  DROP_ANY = 1 << 0,
  DROP_ZERO_READS = 1 << 1,
  DROP_READ_BY_ALL_READERS = 1 << 2,

  NEVER_BLOCK_DROP_OLDEST = DROP_ANY,
  WAIT_FOR_CONSUMPTION_NEVER_DROP = DROP_READ_BY_ALL_READERS,
  WAIT_FOR_CONSUMPTION_OR_DROP_ZERO_READS =
      DROP_ZERO_READS + DROP_READ_BY_ALL_READERS
};

/*! The stream class provides a thread-proof synchronized stream system.
 * Basically, a producer thread calls update() with some data, and one or more
 * consumer threads get copies of the data with read() or tryRead().
 *
 * The template parameter T is expected to have fast constructor and assignment
 * operator.
 */
template <class T>
class Stream : public StreamBase<T> {
  public:

      Stream(const std::string& name, NodeBase* node,
             StreamDropPolicy drop_policy = WAIT_FOR_CONSUMPTION_NEVER_DROP,
             int max_queue_size = 4);

      ~Stream();

      bool update(Timestamp timestamp, T data);

      bool canUpdate() const { return numItemsInQueue() < maxQueueSize(); }

      virtual std::string typeName() const { return media_graph::typeName<T>(); }

      /*! Wakes all waiting threads, making all current and future calls to
       *  push() and pop() fail. 
       */
      virtual void close();

      //! Cancel close(): update, read, and tryRead will behave as normal.
      virtual void open(); 

      StreamDropPolicy drop_policy() const { return drop_policy_; }

      virtual bool unregisterReader(NamedPin* reader);

      Timestamp lastWrittenTimestamp() const { return last_written_timestamp_; }

      int64_t getNumUpdateCalls() const { return next_sequence_id_; }

      int numItemsInQueue() const { return int(buffer_.size()); }
      int maxQueueSize() const { return queue_limit_; }
      bool setMaxQueueSize(const int &size) { queue_limit_ = size; return true; }

  protected:
      virtual bool read(StreamReader<T>* reader,
                        T* data, Timestamp* timestamp, SequenceId *seq);
      virtual bool tryRead(StreamReader<T>* reader,
                           T* data, Timestamp* timestamp, SequenceId* seq);
      virtual bool canRead(SequenceId consumed_until, Timestamp fresher_than) const;

      virtual void markReadAfter(SequenceId seq);

  private:
      struct Entry {
          Entry(Timestamp timestamp, SequenceId sequence_id, T data, int num_reads)
              : timestamp(timestamp), sequence_id(sequence_id),
                data(data), num_reads(num_reads) { }

          Timestamp timestamp;
          SequenceId sequence_id;
          T data;

          // Count the number of times the entry has been read.
          // When all readers read the entry, we can discard it.
          int num_reads;
      };

      bool findAndReadEntry(Timestamp fresher_than, SequenceId* consumed_until, 
                            T* data, Timestamp* timestamp, SequenceId* seq);
      bool findEntry(SequenceId consumed_until, Timestamp fresher_than) const;
      void dropEntries();
      int numLostAndActiveReaders() const {
          return this->numReaders() + num_lost_readers_;
      }

      std::deque<Entry> buffer_;
      int queue_limit_;
      bool closed_;
      ConditionVariable data_available_;
      ConditionVariable slot_available_;

      // The number of readers that disconnected while the stream was operating.
      int num_lost_readers_;

      // Counts the number of calls to update() since last stream opening. Used
      // to assign a unique and monotonic sequence id to each frame.
      int64_t next_sequence_id_;
      StreamDropPolicy drop_policy_;

      // Remember when was the last update(), to avoid going back in time.
      Timestamp last_written_timestamp_;
};

template <class T>
Stream<T>::Stream(const std::string& name, NodeBase* node,
                  StreamDropPolicy drop_policy, int max_queue_size)
    : StreamBase<T>(name, node),
    queue_limit_(max_queue_size),
    closed_(false),
    num_lost_readers_(0),
    next_sequence_id_(0),
    drop_policy_(drop_policy),
    last_written_timestamp_(Timestamp::microSecondsSince1970(0)) {
    this->addGetProperty("NumUpdates", this, &Stream<T>::getNumUpdateCalls);
    this->addGetProperty("NumItemsInQueue", this, &Stream<T>::numItemsInQueue);
    this->addGetSetProperty("MaxQueueSize", this, &Stream<T>::maxQueueSize,
        &Stream<T>::setMaxQueueSize);
}

template <class T>
Stream<T>::~Stream() { close(); }

template<class T>
bool Stream<T>::findEntry(SequenceId consumed_until,
                          Timestamp fresher_than) const {
    for (typename std::deque<Entry>::const_iterator it = buffer_.begin();
         it != buffer_.end(); ++it) {
        if (consumed_until < it->sequence_id && fresher_than < it->timestamp) {
            return true;
        }
    }
    return false;
}

template<class T>
bool Stream<T>::findAndReadEntry(Timestamp fresher_than,
                                 SequenceId* consumed_until, 
                                 T* data, Timestamp* timestamp,
                                 SequenceId* seq) {
    bool found = false;

    for (typename std::deque<Entry>::iterator it = buffer_.begin();
         !found && it != buffer_.end(); ) {
        bool incremented = false;
        if (*consumed_until < it->sequence_id) {
            *consumed_until = it->sequence_id;
            ++(it->num_reads);

            if (fresher_than < it->timestamp) {
                *data = it->data;
                *timestamp = it->timestamp;
                if (seq) {
                    *seq = it->sequence_id;
                }
                found = true;  // Exit loop.
                if ((drop_policy_ & DROP_READ_BY_ALL_READERS) != 0
                    && it->num_reads >= this->numLostAndActiveReaders()) {
                    it = buffer_.erase(it);
                    incremented = true;
                    slot_available_.signal();
                }
            }
        }
        if (!incremented) {
            ++it;
        }
    }
    dropEntries();
    return found;
}

template<class T>
bool Stream<T>::read(StreamReader<T>* reader,
                     T* data, Timestamp* timestamp, SequenceId *seq) {
    if (closed_ || !reader->isConnected()) {
        return false;
    }

    ScopedLock lock(&this->mutex_);

    while (!closed_ && reader->isConnected()
           && !findAndReadEntry(reader->seekPosition(),
                                reader->lastReadSequenceIdPtr(),
                                data, timestamp, seq)) {
        // No data. We need to wait.
        data_available_.wait(&this->mutex_);
    }

    bool success = !closed_ && reader->isConnected();

    return success;
}

template<class T>
bool Stream<T>::tryRead(StreamReader<T>* reader,
                        T* data, Timestamp* timestamp, SequenceId* seq) {
    ScopedLock lock(&this->mutex_);
    bool success = !closed_ && reader->isConnected()
        && findAndReadEntry(reader->seekPosition(),
                            reader->lastReadSequenceIdPtr(),
                            data, timestamp, seq);
    return success;
}

template<class T>
bool Stream<T>::canRead(SequenceId consumed_until, Timestamp fresher_than) const {
    ScopedLock lock(&this->mutex_);
    if (closed_) {
        return false;
    }

    return findEntry(consumed_until, fresher_than);
}

template<class T>
void Stream<T>::markReadAfter(SequenceId seq) {
    for (typename std::deque<Entry>::iterator it = buffer_.begin();
         it != buffer_.end(); ++it) {
        if (it->sequence_id > seq) {
            ++(it->num_reads);
        }
    }
    dropEntries();
}

template <class T>
void Stream<T>::dropEntries() {
    assert(drop_policy_ & (DROP_ANY | DROP_ZERO_READS
                           | DROP_READ_BY_ALL_READERS));
    if (buffer_.size() == 0) {
        return;
    } else if ((drop_policy_ & DROP_ANY) != 0) {
        while (buffer_.size() >= static_cast<unsigned>(queue_limit_)) {
            buffer_.pop_front();
        }
    } else {
        for (typename std::deque<Entry>::iterator it = buffer_.begin();
             it != buffer_.end(); ) {
            if (((drop_policy_ & DROP_ZERO_READS) != 0
                  && it->num_reads == 0)
                || ((drop_policy_ & DROP_READ_BY_ALL_READERS) != 0
                    && it->num_reads >= this->numLostAndActiveReaders())) {
                it = buffer_.erase(it);

                if (buffer_.size() < static_cast<unsigned>(queue_limit_)) {
                    slot_available_.signal();
                }

                break;
            } else {
                ++it;
            }
        }
    }
}

template<class T>
bool Stream<T>::update(Timestamp timestamp, T data) {
    ScopedLock lock(&this->mutex_);

    // Make sure we do not go back in time.
    assert(!(timestamp < last_written_timestamp_));
    if (timestamp < last_written_timestamp_) {
        return false;
    }
    last_written_timestamp_ = timestamp;

    bool success = false;
    if (!closed_) {
        SequenceId sequence_id = next_sequence_id_;
        ++next_sequence_id_;

        dropEntries();
        while (!closed_ && buffer_.size() >= static_cast<unsigned>(queue_limit_)) {
            assert(drop_policy_ != NEVER_BLOCK_DROP_OLDEST);
            slot_available_.wait(&this->mutex_);
            dropEntries();
        }
        if (!closed_) {
            assert(buffer_.size() < static_cast<unsigned>(queue_limit_));

            // Count how many readers are interested in this entry.
            int interested = 0;
            for (int i = 0; i < this->numReaders(); ++i) {
                StreamReader<T>* reader = static_cast<StreamReader<T>*>(
                    this->reader(i));
                if (reader->seekPosition() < timestamp) {
                    interested++;
                    reader->signalActivity();
                } else {
                    *reader->lastReadSequenceIdPtr() = sequence_id;
                }
            }

            if (interested > 0) {
                // There is at least 1 reader that does not want to skip the
                // entry: let's push it. Lost readers are considered as not
                // interested.
                buffer_.push_back(Entry(timestamp, sequence_id, data,
                                        this->numLostAndActiveReaders() - interested));
                data_available_.wakeAll();
            }
            success = true;
        }
    }
    return success;
}

template<class T>
void Stream<T>::close() {
    ScopedLock lock(&this->mutex_);

    buffer_.clear();
    closed_ = true;

    // Let's tell everybody it is no use to wait for us, we're closed.
    data_available_.wakeAll();
    slot_available_.wakeAll();
    for (int i = 0; i < this->numReaders(); ++i) {
        this->reader(i)->signalActivity();
    }
}

template<class T>
void Stream<T>::open() {
    if (closed_) {
        num_lost_readers_ = 0;
        next_sequence_id_ = 0;
    }
    closed_ = false;
}

template<class T>
bool Stream<T>::unregisterReader(NamedPin* reader) {
	if (NamedStream::unregisterReader(reader)) {
		++num_lost_readers_;

                // The disconnected reader might be waiting.
                // Let's wake it.
                static_cast<StreamReader<T>*>(reader)->signalActivity();
                data_available_.wakeAll();
		return true;
	}
	return false;
}

}  // media_graph

#endif
