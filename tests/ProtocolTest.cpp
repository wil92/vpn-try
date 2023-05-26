//
// Created by Guillermo on 25.05.23.
//

#include <arpa/inet.h>

#include "gtest/gtest.h"
#include "protocol.cpp"

TEST (ProtocolTest, ToAndFromProtocolMessage) {
    char text[10] = "test";

    // to
    char data[50];
    int dataLen;

    uint32_t addr = inet_addr("255.255.255.255");

    uint16_t sin_port = htons(1254);

    protocol::to(&addr, &sin_port, text, (int) strlen(text), data, &dataLen);
//    std::cout << data << std::endl;

    // from
    char result[50];
    int resultLen;

    uint32_t addrFrom;
    uint16_t sin_portFrom;

    protocol::from(&addrFrom, &sin_portFrom, data, dataLen, result, &resultLen);
//    std::cout << result << std::endl;

    ASSERT_EQ (addr, addrFrom);
    ASSERT_EQ (sin_port, sin_portFrom);
    ASSERT_EQ ((int) strlen(text), resultLen);
    for (int i = 0; i < resultLen; ++i) {
        ASSERT_EQ (result[i], text[i]);
    }
}

TEST (ProtocolTest, ToAndFromProtocolMessageWithoutNetwork) {
    char text[10] = "test";

    // to
    char data[50];
    int dataLen;

    protocol::to(nullptr, nullptr, text, (int) strlen(text), data, &dataLen);

    // from
    char result[50];
    int resultLen;

    uint32_t addrFrom;
    uint16_t sin_portFrom;

    protocol::from(&addrFrom, &sin_portFrom, data, dataLen, result, &resultLen);

    ASSERT_EQ ((int) strlen(text), resultLen);
    for (int i = 0; i < resultLen; ++i) {
        ASSERT_EQ (result[i], text[i]);
    }
}

TEST (ProtocolTest, ToProtocolMessage) {
    char text[10] = "test";
    char expected[10] = "dGVzdA==";
    int expectedLen = (int) strlen(expected);

    char result[50];
    int resultLen;

    uint32_t addr = inet_addr("255.255.255.255");

    uint16_t sin_port = htons(1254);

    protocol::to(&addr, &sin_port, text, (int) strlen(text), result, &resultLen);
//    std::cout << result << std::endl;

    // check protocol separators
    ASSERT_EQ (result[6], '.');
    ASSERT_EQ (result[10], '.');

    // validate addr
    char addrRes[10] = "/////D";
    for (int i = 0; i < (int) strlen(addrRes); ++i) {
        ASSERT_EQ (result[i], addrRes[i]);
    }

    // validate port
    char portRes[10] = "EYO";
    for (int i = 0; i < (int) strlen(portRes); ++i) {
        ASSERT_EQ (result[i + 7], portRes[i]);
    }

    // validate message
    ASSERT_EQ (expectedLen, resultLen - 11);
    for (int i = 0; expected[i] != '\0'; ++i) {
        ASSERT_EQ (expected[i], result[i + 11]);
    }
}

TEST (ProtocolTest, ToProtocolMessageWithoutNetwork) {
    char text[10] = "test";
    char expected[10] = "dGVzdA==";

    char result[50];
    int resultLen;

    protocol::to(nullptr, nullptr, text, (int) strlen(text), result, &resultLen);
//    std::cout << result << std::endl;

    // validate message
    ASSERT_EQ ((int) strlen(expected), resultLen);
    for (int i = 0; expected[i] != '\0'; ++i) {
        ASSERT_EQ (expected[i], result[i]);
    }
}

TEST (ProtocolTest, FromProtocolMessage) {
    char data[25] = "/////D.EYO.dGVzdA==";
    char expected[10] = "test";
    int expectedLen = (int) strlen(expected);

    char result[50];
    int resultLen;

    uint32_t addr;
    uint16_t sin_port;

    protocol::from(&addr, &sin_port, data, (int) strlen(data), result, &resultLen);

    // validate addr
    ASSERT_EQ (addr, inet_addr("255.255.255.255"));

    // validate port
    ASSERT_EQ (sin_port, htons(1254));

    // validate message
    ASSERT_EQ (expectedLen, resultLen);
    for (int i = 0; expected[i] != '\0'; ++i) {
        ASSERT_EQ (expected[i], result[i]);
    }
}

TEST (ProtocolTest, FromProtocolMessageWithoutAddress) {
    char data[25] = "dGVzdA==";
    char expected[10] = "test";
    int expectedLen = (int) strlen(expected);

    char result[50];
    int resultLen;

    uint32_t addr;
    uint16_t sin_port;

    protocol::from(&addr, &sin_port, data, (int) strlen(data), result, &resultLen);

    // validate message
    ASSERT_EQ (expectedLen, resultLen);
    for (int i = 0; expected[i] != '\0'; ++i) {
        ASSERT_EQ (expected[i], result[i]);
    }
}
