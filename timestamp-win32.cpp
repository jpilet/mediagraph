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
#include "timestamp.h"

#define WIN32_LEAN_AND_MEAN
#include <time.h>
#include <windows.h>

namespace {
// GetSystemTimeAsFileTime is based in 1601.
// Our reference is 1970. This is the difference between both epochs.
// Unit: 100 nanoseconds
const int64_t kDeltaEpoch = 11644473600000000ULL;

}  // namespace

void Duration::sleep() const {
    if (duration_ <= 0) { return; }

    Timestamp deadline = Timestamp::now() + *this;
    if (*this > Duration::microSeconds(2000)) { Sleep(DWORD(duration_ / 1000)); }

    while (Timestamp::now() < deadline) {
        // wait..
    }
}

Timestamp Timestamp::now() {
    FILETIME time;
    GetSystemTimeAsFileTime(&time);
    return Timestamp(
        (int64_t(time.dwLowDateTime) + (int64_t(time.dwHighDateTime) << 32) - kDeltaEpoch) / 10);
}

std::string Timestamp::asString(const char* strftime_format) const {
    struct tm t;
    time_t seconds_since_epoch = epoch_ / 1000000;

    gmtime_s(&t, &seconds_since_epoch);

    char result[256];
    strftime(result, sizeof(result), strftime_format, &t);
    return std::string(result);
}
