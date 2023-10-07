#include <gtest/gtest.h>

#include "../src/urlencode.h"

using namespace std::literals;

TEST(UrlEncodeTestSuite, OrdinaryCharsAreNotEncoded) {
    EXPECT_EQ(UrlEncode("hello"sv), "hello"s);
    EXPECT_EQ(UrlEncode("~hello!"sv), "~hello%21"s);
    EXPECT_EQ(UrlEncode("[hello]"sv), "%5Bhello%5D"s);
    EXPECT_EQ(UrlEncode("(he&l*lo)"sv), "%28he%26l%2Alo%29"s);
}

/* Напишите остальные тесты самостоятельно */
