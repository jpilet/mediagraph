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
#ifndef BASE_TIMESTAMP_H
#define BASE_TIMESTAMP_H

#include <stdint.h>
#include <string>

class Timestamp;

//! Represents a relative time period.
//! A duration can be obtained by:
//!  - specifying a constant duration in a specified unit;
//!  - subtracting two Timestamp objects.
//!
//! The default constructor gives a duration or length 0.
class Duration {
  public:
    Duration() { duration_ = 0; }

    static Duration seconds(double sec) {
        return Duration(int64_t(sec * 1e6));
    }
    static Duration milliSeconds(double msec) {
        return Duration(int64_t(msec * 1e3));
    }
    static Duration microSeconds(int64_t microsec) {
        return Duration(microsec);
    }

    int64_t microSeconds() const { return duration_; }
    int64_t milliSeconds() const { return duration_ / 1000; }
    double seconds() const { return double(duration_) * 1e-6; }

    //! Pause execution of the current thread for the specified duration.
    //! The caller is guaranteed to be stopped for at least the duration,
    //! but it could be more. Expect a few milliseconds.
    //! To wait for a short and more acurate time, call Timestamp::now()
    //! until it reaches the time you want.
    void sleep() const;

    Duration abs() const { return Duration(duration_ > 0 ? duration_ : -duration_); }

    Duration operator + (Duration a) {
        return Duration(duration_ + a.duration_);
    }

    Duration operator - (Duration a) {
        return Duration(duration_ - a.duration_);
    }

    Duration operator * (double factor) {
        return Duration(static_cast<int64_t>(duration_ * factor));
    }

    Duration operator * (int64_t factor) {
        return Duration(duration_ * factor);
    }

    bool operator < (Duration a) const { return duration_ < a.duration_; }
    bool operator > (Duration a) const { return duration_ > a.duration_; }
    bool operator <= (Duration a) const { return duration_ <= a.duration_; }
    bool operator >= (Duration a) const { return duration_ >= a.duration_; }
    bool operator == (Duration a) const { return duration_ == a.duration_; }

  private:
    explicit Duration(int64_t microsec) : duration_(microsec) { }

    int64_t duration_;

    friend class Timestamp;
};

//! Represent the time and date at which an event occured.
//!
//! The internal accuracy varies from a system to another.
//! The unit test checks that it is at least 100 micro-sec.
//! The internal unit is microseconds.
class Timestamp {
  public:
    //! Create a timestamp containing the creation time.
    Timestamp() { *this = now(); }

	//! Returns a timestamp containing the current time.
    static Timestamp now();

    static Timestamp microSecondsSince1970(int64_t epoch) {
        return Timestamp(epoch);
    }

    int64_t microSecondsSince1970() const { return epoch_; }

    bool operator < (Timestamp b) const { return epoch_ < b.epoch_; }
    bool operator > (Timestamp b) const { return epoch_ > b.epoch_; }
    bool operator <= (Timestamp b) const { return epoch_ <= b.epoch_; }
    bool operator >= (Timestamp b) const { return epoch_ >= b.epoch_; }
    bool operator == (Timestamp b) const { return epoch_ == b.epoch_; }

    Duration operator - (Timestamp t) const {
        return Duration(epoch_ - t.epoch_);
    }

    Timestamp operator + (Duration d) const {
        return Timestamp(epoch_ + d.duration_);
    }

    Timestamp operator - (Duration d) const {
        return Timestamp(epoch_ - d.duration_);
    }

    Timestamp& operator += (Duration d) {
        epoch_ += d.duration_;
        return *this;
    }

    Timestamp& operator -= (Duration d) {
        epoch_ -= d.duration_;
        return *this;
    }

    //! Returns a string to represent the time and date.
    //! The time is printed as UTC time.
    std::string asString(const char *strftime_format = "%Y.%m.%d - %H:%M:%S") const;

  private:
    explicit Timestamp(int64_t t) : epoch_(t) { }

    // Unit: microseconds (1e-6 seconds) elapsed since Jan. 1st 1970, UTC.
    int64_t epoch_;
};

#endif  // BASE_TIMESTAMP_H
