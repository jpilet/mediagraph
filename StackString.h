#pragma once

#include <cstring>

namespace
{
    template<int MaxLen>
    class StackString
    {
    public:
        StackString(std::initializer_list<const char*> stringList)
        {
            size_t dataLen = 0;
            for (auto str: stringList)
            {
                const size_t strLen = strlen(str);

                if (dataLen + strLen >= MaxLen - 1) { break; }

                for (size_t i=0; i < strLen; ++i)
                {
                    m_data[dataLen + i] = str[i];
                }

                dataLen += strLen;
            }

            m_data[dataLen] = 0; // end character
        }

        const char* c_str() const { return m_data.data(); }
        operator const char*() const { return c_str(); }

    private:
        std::array<char, MaxLen>  m_data;
    };
}