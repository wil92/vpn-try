//
// Created by Guillermo on 25.05.23.
//

#include "gtest/gtest.h"
#include "base64.cpp"

#include <string.h>

TEST (Base64Test, EncodeAndDecodeText) {
    char text[10] = "test";
    int textLen = strlen(text);

    char result[10];
    int resultLen;
    char result2[10];

    base64::encode(text, (int) strlen(text), result, &resultLen);
    base64::decode(result, resultLen, result2, &resultLen);

    ASSERT_EQ (textLen, resultLen);
    for (int i = 0; text[i] != '\0'; ++i) {
        ASSERT_EQ (text[i], result2[i]);
    }
}

TEST (Base64Test, EncodeTextWithResidual) {
    char text[10] = "test";
    char expected[10] = "dGVzdA==";
    int expectedLen = (int) strlen(expected);

    char result[10];
    int resultLen;
    base64::encode(text, (int) strlen(text), result, &resultLen);

    ASSERT_EQ (expectedLen, resultLen);
    for (int i = 0; expected[i] != '\0'; ++i) {
        ASSERT_EQ (expected[i], result[i]);
    }
}

TEST (Base64Test, DecodeTextWithResidual) {
    char text[10] = "dGVzdA==";
    char expected[10] = "test";
    int expectedLen = (int) strlen(expected);

    char result[10];
    int resultLen;
    base64::decode(text, (int) strlen(text), result, &resultLen);

    ASSERT_EQ (expectedLen, resultLen);
    for (int i = 0; expected[i] != '\0'; ++i) {
        ASSERT_EQ (expected[i], result[i]);
    }
}

TEST (Base64Test, EncodeTextWithoutResidual) {
    char text[150] = "lkjsalkdfjlaksdjf890u72qnlkf54654 5+f+s4sdfg+sdfg 4sd+g4+sdf4g+4KLHKHKJHSAKFHKJHjkhgkjhgKJHKJHSAKDFH29";
    char expected[150] = "bGtqc2Fsa2Rmamxha3NkamY4OTB1NzJxbmxrZjU0NjU0IDUrZitzNHNkZmcrc2RmZyA0c2QrZzQrc2RmNGcrNEtMSEtIS0pIU0FLRkhLSkhqa2hna2poZ0tKSEtKSFNBS0RGSDI5";
    int expectedLen = (int) strlen(expected);

    char result[150];
    int resultLen;
    base64::encode(text, (int) strlen(text), result, &resultLen);

    ASSERT_EQ (expectedLen, resultLen);
    for (int i = 0; expected[i] != '\0'; ++i) {
        ASSERT_EQ (expected[i], result[i]);
    }
}

TEST (Base64Test, DecodeTextWithoutResidual) {
    char text[150] = "bGtqc2Fsa2Rmamxha3NkamY4OTB1NzJxbmxrZjU0NjU0IDUrZitzNHNkZmcrc2RmZyA0c2QrZzQrc2RmNGcrNEtMSEtIS0pIU0FLRkhLSkhqa2hna2poZ0tKSEtKSFNBS0RGSDI5";
    char expected[150] = "lkjsalkdfjlaksdjf890u72qnlkf54654 5+f+s4sdfg+sdfg 4sd+g4+sdf4g+4KLHKHKJHSAKFHKJHjkhgkjhgKJHKJHSAKDFH29";
    int expectedLen = (int) strlen(expected);

    char result[150];
    int resultLen;
    base64::decode(text, (int) strlen(text), result, &resultLen);

    ASSERT_EQ (expectedLen, resultLen);
    for (int i = 0; expected[i] != '\0'; ++i) {
        ASSERT_EQ (expected[i], result[i]);
    }
}
