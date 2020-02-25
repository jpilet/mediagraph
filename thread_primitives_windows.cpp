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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class MutexPrivate {
  public:
    MutexPrivate() { InitializeCriticalSection(&mutex_); locked_ = false;}
    ~MutexPrivate() { DeleteCriticalSection(&mutex_); }
    void lock() { EnterCriticalSection(&mutex_); locked_ = true; }
    void unlock() { locked_ = false; LeaveCriticalSection(&mutex_); }
	bool tryLock() {
        // TryEnterCriticalSection returns true if the critical section is
        // already locked by the same thread. To make tryLock behaves as a
        // mutex, we do an additional check.
		if (!locked_ && TryEnterCriticalSection(&mutex_) != 0) {
			locked_ = true;
			return true;
		}
		return false;
	}

    CRITICAL_SECTION& mutex() { return mutex_; }

  private:
    CRITICAL_SECTION mutex_;
	bool locked_;
};

Mutex::Mutex() { impl_ = new MutexPrivate(); }
Mutex::~Mutex() { delete impl_; }
void Mutex::lock() { impl_->lock(); }
void Mutex::unlock() { impl_->unlock(); }
bool Mutex::tryLock() { return impl_->tryLock(); }

class ConditionVariablePrivate {
  public:
    ConditionVariablePrivate() {
        InitializeConditionVariable(&condition_variable_);
    }
    void signal() {
        WakeConditionVariable(&condition_variable_);
    }
    void wakeAll() {
        WakeAllConditionVariable(&condition_variable_);
    }
    void wait(Mutex* mutex) {
        SleepConditionVariableCS(&condition_variable_,
                                 &mutex->impl_->mutex(),
                                 INFINITE);
    }
  private:
    CONDITION_VARIABLE condition_variable_;
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

Thread::Thread() {
    thread_handle = 0;
}

Thread::~Thread() {
    if (thread_handle) {
        CloseHandle(static_cast<HANDLE>(thread_handle));
    }
}

bool Thread::isRunning() const {
    DWORD exitCode;
    return (thread_handle != NULL)
        && GetExitCodeThread(thread_handle, &exitCode)
        && (exitCode == STILL_ACTIVE);
}

bool Thread::start(int (*func)(void *), void *ptr) {
    if (thread_handle != NULL) {
        DWORD exitCode;
        if (GetExitCodeThread(thread_handle, &exitCode) && exitCode == STILL_ACTIVE) {
            return false;
        }
        CloseHandle(thread_handle);
        thread_handle = NULL;
    }

    DWORD id;
    thread_handle = CreateThread(
        NULL, 0, (LPTHREAD_START_ROUTINE)func, ptr, 0, &id);
    return NULL != thread_handle;
}

int Thread::waitForTermination() {
    if (thread_handle) {
        WaitForSingleObject(static_cast<HANDLE>(thread_handle), INFINITE);
        DWORD ret_val;
        GetExitCodeThread(static_cast<HANDLE>(thread_handle), &ret_val);
		CloseHandle(static_cast<HANDLE>(thread_handle));
		thread_handle = 0;
        return int(ret_val);
    }
    return -1;
}

// SetThreadName, as found on
// http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
//
// Usage: SetThreadName (-1, "MainThread");
//
const DWORD MS_VC_EXCEPTION=0x406D1388;

#pragma pack(push,8)
typedef struct THREADNAME_INFO
{
   DWORD dwType; // Must be 0x1000.
   LPCSTR szName; // Pointer to name (in user addr space).
   DWORD dwThreadID; // Thread ID (-1=caller thread).
   DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void Thread::setCurrentName(const char *name) {
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = name;
    info.dwThreadID = -1;
    info.dwFlags = 0;

    __try
    {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR),
                       (ULONG_PTR*)&info );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
}
