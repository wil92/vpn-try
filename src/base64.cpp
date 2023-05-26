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

#ifndef MESSAGE_TEST_BASE64_CPP
#define MESSAGE_TEST_BASE64_CPP

#include <map>

struct base64 {
private:
    constexpr static char alp[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

public:
    static char getCode(int index) {
        return alp[index];
    }

    static int getIndex(char code) {
        for (int i = 0; i < 64; ++i) {
            if (code == alp[i]) {
                return i;
            }
        }
        return -1;
    }

    static void encode(const char *text, int len, char *encoded, int *resultLen) {
        *resultLen = 0;
        for (int i = 0; i < len; i += 3) {
            int buff = (int) text[i] << 16, split = 18, bits = 8;
            if (i + 1 < len) {
                buff += (int) (text[i + 1]) << 8;
                bits = 16;
            }
            if (i + 2 < len) {
                buff += (int) (text[i + 2]);
                bits = 24;
            }
            for (int j = 0; j < 4; ++j) {
                encoded[(*resultLen)++] = j * 6 <= bits ? alp[(buff >> split) & 63] : '=';
                split -= 6;
            }
        }
        encoded[*resultLen] = '\0';
    }

    static void decode(const char *text, int len, char *encoded, int *resultLen) {
        *resultLen = 0;

        std::map<char, int> m;
        for (int i = 0; i < 65; ++i) {
            m.insert({alp[i], i});
        }

        for (int i = 0; i < len; i += 4) {
            int buff = (m[text[i]] << 18) + (m[text[i + 1]] << 12), split = 16, toTake = 3;
            if (text[i + 3] != '=') {
                buff += m[text[i + 3]];
            } else {
                toTake = 2;
            }
            if (text[i + 2] != '=') {
                buff += m[text[i + 2]] << 6;
            } else {
                toTake = 1;
            }
            for (int j = 0; j < toTake; ++j) {
                encoded[(*resultLen)++] = (char) ((buff >> split) & 511);
                split -= 8;
            }
        }
        encoded[*resultLen] = '\0';
    }
};

#endif //MESSAGE_TEST_BASE64_CPP
