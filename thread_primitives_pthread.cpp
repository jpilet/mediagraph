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
// julien.pilet@aptarism.com, 2012.

#include "thread_primitives.h"

#include <pthread.h>
#include <signal.h>

class MutexPrivate {
  public:
    MutexPrivate() { pthread_mutex_init(&mutex_, 0); }
    ~MutexPrivate() { pthread_mutex_destroy(&mutex_); }
    void lock() { pthread_mutex_lock(&mutex_); }
    void unlock() { pthread_mutex_unlock(&mutex_); }
    bool tryLock() { return pthread_mutex_trylock(&mutex_) == 0; }

    pthread_mutex_t& mutex() { return mutex_; }

  private:
    pthread_mutex_t mutex_; 
};

Mutex::Mutex() { impl_ = new MutexPrivate(); }
Mutex::~Mutex() { delete impl_; }
void Mutex::lock() { impl_->lock(); }
void Mutex::unlock() { impl_->unlock(); }
bool Mutex::tryLock() { return impl_->tryLock(); }

class ConditionVariablePrivate {
  public:
    ConditionVariablePrivate() {
        pthread_cond_init(&condition_variable_, 0);
    }
    void signal() {
        pthread_cond_signal(&condition_variable_);
    }
    void wakeAll() {
        pthread_cond_broadcast(&condition_variable_);
    }
    void wait(Mutex* mutex) {
        pthread_cond_wait(&condition_variable_,
                          &mutex->impl_->mutex());
    }
  private:
    pthread_cond_t condition_variable_;
};

ConditionVariable::ConditionVariable() {
    impl_ = new ConditionVariablePrivate();
}

ConditionVariable::~ConditionVariable() {
    delete impl_;
}

void ConditionVariable::signal() { impl_->signal(); }
void ConditionVariable::wakeAll() { impl_->wakeAll(); }
void ConditionVariable::wait(Mutex* mutex) { impl_->wait(mutex); }

pthread_t* cast(void *ptr) {
    return static_cast<pthread_t *>(ptr);
}

Thread::Thread() {
    thread_handle = 0;
}

Thread::~Thread() {
    if (thread_handle) {
        // not sure if this is correct.
        pthread_detach(*cast(thread_handle));
        delete cast(thread_handle);
    }
}

bool Thread::start(int (*func)(void *), void *ptr) {
    if (thread_handle) {
        // pthread kill is used to send a message to the thread.
        // The message "0" is special: no message is sent, only error checking
        // is done. The following line therefore just checks if the thread
        // is alive.
        if (pthread_kill(*cast(thread_handle), 0) == 0) {
            return false;  // already running.
        } else {
            // Dead thread.
            delete cast(thread_handle);
            thread_handle = 0;
        }
    }

    thread_handle = new pthread_t;
    return pthread_create(cast(thread_handle), 0, (void* (*)(void *))func, ptr) == 0;
}

bool Thread::isRunning() const {
    return thread_handle && (pthread_kill(*cast(thread_handle), 0) == 0);
}

int Thread::waitForTermination() {

    if (thread_handle) {
        void* ret_val;
        pthread_join(*cast(thread_handle), &ret_val);
        delete cast(thread_handle);
        thread_handle = 0;
        return (int)((long)ret_val);
    }
    return -1;
}

void Thread::setCurrentName(const char *name) {
    pthread_setname_np(name);
}
