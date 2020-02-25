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
#ifndef THREAD_PRIMITIVES_H
#define THREAD_PRIMITIVES_H

class ConditionVariablePrivate;
class MutexPrivate;

class Mutex {
  public:
    Mutex();
	~Mutex();
    void lock();
    bool tryLock();
    void unlock();

    MutexPrivate* impl_;
};

//! Aquire a lock on the mutex passed to the constructor.
//! The lock is maintained while the ScopedLock object is alive.
//! The lock is released uppon destruction.
class ScopedLock {
  public:
    ScopedLock(Mutex* mutex) : mutex_(mutex) { mutex->lock(); }
    ~ScopedLock() { mutex_->unlock(); }

  private:
    ScopedLock(const ScopedLock&) { }
    ScopedLock& operator = (const ScopedLock&) { return *this; }
    Mutex* mutex_;
};

class ConditionVariable {
  public:
    ConditionVariable();
    ~ConditionVariable();
    
    void signal();
    void wakeAll();
    void wait(Mutex* mutex);

  private:
    ConditionVariablePrivate* impl_;
};

class Thread {
  public:
    Thread();
    ~Thread();

    bool start(int (*func)(void *), void *ptr);

    bool isRunning() const;

    // Returns the thread exit value, as returned by 'func' above.
    // returns -1 if the thread has never been started.
    int waitForTermination();

    static void setCurrentName(const char *name);

  private:
    void* thread_handle;
};


#endif  // THREAD_PRIMITIVES_H
