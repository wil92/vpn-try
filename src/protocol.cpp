/*
 * Copyright 2023 Guillermo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MESSAGE_TEST_PROTOCOL_CPP
#define MESSAGE_TEST_PROTOCOL_CPP

#include "base64.cpp"

struct protocol {
private:
    constexpr static int protocolShift = 11;
public:
    static void to(const unsigned int *addr, const unsigned short int *port, char *msg, int msgLen, char *buffer, int *bufferLen) {
        int shiftData = 0;
        if (addr != nullptr && port != nullptr) {
            shiftData = protocolShift;
            buffer[6] = buffer[10] = '.';

            // encode addr
            for (int i = 0, shift = 0; i < 6; ++i, shift += 6) {
                buffer[i] = base64::getCode((((*addr)) >> shift) & 63);
            }

            // encode port
            for (int i = 7, shift = 0; i <= 9; ++i, shift += 6) {
                buffer[i] = base64::getCode(((*port) >> shift) & 63);
            }
        }

        // encode msg
        base64::encode(msg, msgLen, buffer + shiftData, bufferLen);

        *bufferLen += shiftData;
        buffer[*bufferLen] = '\0';
    }

    static void
    from(unsigned int *addr, unsigned short int *port, const char *data, int dataLen, char *msg, int *msgLen) {
        int shiftData = 0;
        if (data[6] == '.') {
            // data with network information
            shiftData = protocolShift;

            // decode addr
            *addr = 0;
            for (int i = 0, shift = 0; i < 6; ++i, shift += 6) {
                *addr += base64::getIndex(data[i]) << shift;
            }

            // decode port
            *port = 0;
            for (int i = 7, shift = 0; i <= 9; ++i, shift += 6) {
                *port += base64::getIndex(data[i]) << shift;
            }
        }
        // decode message
        base64::decode(data + shiftData, dataLen - shiftData, msg, msgLen);
    }
};

#endif //MESSAGE_TEST_PROTOCOL_CPP
