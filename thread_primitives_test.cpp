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

#include <gtest/gtest.h>

#include "thread_primitives.h"
#include "timestamp.h"

static int return_arg(void *ptr) {
    return (int) ((long)ptr);
}

TEST(ThreadTest, JoinValue) {
    Thread thread;
    EXPECT_TRUE(thread.start(return_arg, (void *)7));
    thread.waitForTermination();
    EXPECT_FALSE(thread.isRunning());
}

TEST(ThreadTest, BasicCreation) {
    // Make sure the constructor and destructors work on unsued objects.
    Thread unstarted;
}

static int never_returns(void *ptr) {
    while(1) {
    }
}

TEST(ThreadTest, DeleteWhileRunning) {
    Thread thread;
    thread.start(never_returns, 0);

    // When thread goes out of scope, the thread should not prevent the test to
    // timeout.
}

static int wait_a_bit(void *ptr) {
    Duration::milliSeconds(10).sleep();
    if (ptr) {
        *static_cast<int *>(ptr) = 1;
    }
    return 0;
}

TEST(ThreadTest, MultipleStarts) {
    Thread thread;
    int done = 0;
    EXPECT_TRUE(thread.start(wait_a_bit, &done));
    EXPECT_FALSE(thread.start(wait_a_bit, 0)); // already running

    while (!done) {
        Duration::milliSeconds(2).sleep();
    }
    Duration::milliSeconds(2).sleep();

    // should work again.
    ASSERT_TRUE(thread.start(wait_a_bit, 0));

    thread.waitForTermination();
    EXPECT_FALSE(thread.isRunning());
}

// Verify the behavior of isRunning().
TEST(ThreadTest, IsRunning) {
    Thread thread;
    // When instanciated, the thread shoud not be running.
    EXPECT_FALSE(thread.isRunning());

	int done = 0;
    EXPECT_TRUE(thread.start(wait_a_bit, &done));

    // We just started the thread, it should be running.
    EXPECT_TRUE(thread.isRunning());

	while (!done) {
		Duration::milliSeconds(2).sleep();
	}
	Duration::milliSeconds(2).sleep();

    // We waited long enough: the thread should have returned.
    EXPECT_FALSE(thread.isRunning());

    thread.waitForTermination();
    EXPECT_FALSE(thread.isRunning());
}
