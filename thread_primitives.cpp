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

#include <chrono>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <thread>

class MutexPrivate {
  public:
    void lock() { mutex_.lock(); }
    bool tryLock() { return mutex_.try_lock(); }
    void unlock() { mutex_.unlock(); }

    std::mutex& mutex() { return mutex_; }

  private:
    std::mutex mutex_;
};

Mutex::Mutex() { impl_ = new MutexPrivate(); }
Mutex::~Mutex() { delete impl_; }
void Mutex::lock() { impl_->lock(); }
void Mutex::unlock() { impl_->unlock(); }
bool Mutex::tryLock() { return impl_->tryLock(); }

class ConditionVariablePrivate {
  public:
    void signal() {
        condvar_.notify_one();
    }
    void wakeAll() {
        condvar_.notify_all();
    }
    void wait(Mutex* mutex) {
        std::unique_lock<std::mutex> ulock(mutex->impl_->mutex(), std::defer_lock);
        condvar_.wait(ulock);
    }
  private:
    std::condition_variable condvar_;
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

Thread::~Thread() {
    if (thread_.joinable()) {
        thread_.detach();
    }
}

bool Thread::start(int (*func)(void *), void *ptr) {
    if (isRunning()) {
        return false;
    }

    waitForTermination(); // thread may still be finishing, make sure we join it

    // Wrap the function in a lambda to signal when the thread has finished
    // processing through a future
    std::packaged_task<void(void *)> task(func);
    running_future_ = task.get_future();

    thread_ = std::thread(std::move(task), ptr);

    return true;
}

bool Thread::isRunning() const {
    // (from c++ docs)
    // vallid() == true: This is the case only for futures that were not
    // default-constructed or moved from  until the first time get() or share()
    // is called.
    if (!running_future_.valid()) {
        return false;
    }

    const auto status = running_future_.wait_for(std::chrono::seconds(0));
    return status != std::future_status::ready;
}

void Thread::waitForTermination() {
    if (thread_.joinable()) {
        thread_.join();
    }

    // deal with the future accordingly
    if (running_future_.valid()) {
        running_future_.get();
    }
}

