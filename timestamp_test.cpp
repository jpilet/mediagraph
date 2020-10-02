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

#include "timestamp.h"

TEST(TimestampTest, ConstructNow) {
    Timestamp timeAtStart(Timestamp::now());
    Timestamp alsoTimeAtStart;

    // We expect a timer accuracy of .1 millisecond.
    double difference = (alsoTimeAtStart - timeAtStart).seconds();
    EXPECT_LE(0, difference);
    EXPECT_NEAR(0.0, difference, 1e-4);
}

TEST(TimestampTest, CheckSmallestIncrement) {
    Timestamp timeAtStart(Timestamp::now());

    Timestamp aBitLater;
    while (!(timeAtStart < aBitLater)) { aBitLater = Timestamp::now(); }

    // We expect a timer accuracy of .1 millisecond.
    double difference = (aBitLater - timeAtStart).seconds();
    EXPECT_LT(0, difference);
    EXPECT_NEAR(0.0, difference, 1e-3);
}

TEST(TimestampTest, WaitLoop) {
    Timestamp timeAtStart(Timestamp::now());
    Timestamp later(Timestamp::now() + Duration::milliSeconds(30));

    while (Timestamp::now() < later) {}
    Timestamp after;

    Duration waiting_time(after - timeAtStart);

    // In theory, we should have waited for 30 milliseconds.
    // This test expects an accuracy of .1 millisecond.
    EXPECT_NEAR(30e-3, waiting_time.seconds(), 1e-4);
}

TEST(TimestampTest, OneSecondConstructors) {
    // Different ways of measuring a second.
    Duration delta(Duration::milliSeconds(1000));
    Duration delta2(Duration::seconds(1.0));
    Duration delta3(Duration::microSeconds(1000000));

    EXPECT_EQ(1000000, delta.microSeconds());
    EXPECT_EQ(1000000, delta2.microSeconds());
    EXPECT_EQ(1000000, delta3.microSeconds());
}

TEST(TimestampTest, Arithmetic) {
    Timestamp timeAtStart(Timestamp::now());
    Duration delta(Duration::milliSeconds(1000));
    Timestamp later(timeAtStart + delta);
    Timestamp copy(timeAtStart);
    copy += delta;
    EXPECT_FALSE(copy < later);
    EXPECT_FALSE(later < copy);
}

TEST(TimestampTest, Sleep) {
    const Duration wantedWaitTime(Duration::milliSeconds(12));

    Timestamp before(Timestamp::now());
    wantedWaitTime.sleep();
    Timestamp after(Timestamp::now());

    Duration actualWaitTime = after - before;

    // We want to be sure we waited at least the specified time.
    EXPECT_LE(wantedWaitTime.microSeconds(), actualWaitTime.microSeconds());
}
