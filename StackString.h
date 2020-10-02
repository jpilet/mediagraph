#pragma once

#include <cstring>

namespace {
template <int MaxLen> class StackString {
public:
    StackString(std::initializer_list<const char*> stringList) {
        size_t dataLen = 0;
        for (auto str : stringList) {
            const size_t newCharsToAdd = std::min(MaxLen - 1 - dataLen, strlen(str));

            for (size_t i = 0; i < newCharsToAdd; ++i) { m_data[dataLen + i] = str[i]; }

            dataLen += newCharsToAdd;
            if (dataLen >= MaxLen - 1) { break; }
        }

        m_data[dataLen] = 0;  // end character
    }

    const char* c_str() const { return m_data.data(); }
    operator const char*() const { return c_str(); }

private:
    std::array<char, MaxLen> m_data;
};
}  // namespace