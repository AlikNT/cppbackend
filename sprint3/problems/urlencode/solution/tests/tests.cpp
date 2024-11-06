#include <gtest/gtest.h>

#include "../src/urlencode.h"

using namespace std::literals;

TEST(UrlEncodeTestSuite, OrdinaryCharsAreNotEncoded) {
    EXPECT_EQ(UrlEncode("hello"sv), "hello"s);
}

TEST(UrlEncodeTestSuite, SpaceIsEncodedAsPlus) {
    EXPECT_EQ(UrlEncode("Hello World"sv), "Hello+World"s);
    EXPECT_EQ(UrlEncode(" "sv), "+"s);
}

TEST(UrlEncodeTestSuite, ReservedCharsAreEncoded) {
    EXPECT_EQ(UrlEncode("!"sv), "%21"s);
    EXPECT_EQ(UrlEncode("#"sv), "%23"s);
    EXPECT_EQ(UrlEncode("$"sv), "%24"s);
    EXPECT_EQ(UrlEncode("&"sv), "%26"s);
    EXPECT_EQ(UrlEncode("'"sv), "%27"s);
    EXPECT_EQ(UrlEncode("("sv), "%28"s);
    EXPECT_EQ(UrlEncode(")"sv), "%29"s);
    EXPECT_EQ(UrlEncode("*"sv), "%2A"s);
    EXPECT_EQ(UrlEncode("+"sv), "%2B"s);
    EXPECT_EQ(UrlEncode(","sv), "%2C"s);
    EXPECT_EQ(UrlEncode("/"sv), "%2F"s);
    EXPECT_EQ(UrlEncode(":"sv), "%3A"s);
    EXPECT_EQ(UrlEncode(";"sv), "%3B"s);
    EXPECT_EQ(UrlEncode("="sv), "%3D"s);
    EXPECT_EQ(UrlEncode("?"sv), "%3F"s);
    EXPECT_EQ(UrlEncode("@"sv), "%40"s);
    EXPECT_EQ(UrlEncode("["sv), "%5B"s);
    EXPECT_EQ(UrlEncode("]"sv), "%5D"s);
}

TEST(UrlEncodeTestSuite, NonAsciiCharsAreEncoded) {
    EXPECT_EQ(UrlEncode("\x80"sv), "%80"s);     // Код 128
    EXPECT_EQ(UrlEncode("\xFF"sv), "%FF"s);     // Код 255
}

TEST(UrlEncodeTestSuite, ControlCharsAreEncoded) {
    EXPECT_EQ(UrlEncode("\x00"sv), "%00"s);     // Код 0
    EXPECT_EQ(UrlEncode("\x1F"sv), "%1F"s);     // Код 31
}

TEST(UrlEncodeTestSuite, MixedStringEncoding) {
    EXPECT_EQ(UrlEncode("Hello World!"sv), "Hello+World%21"s);    // Пробел и восклицательный знак
    EXPECT_EQ(UrlEncode("abc*"sv), "abc%2A"s);                    // Обычные символы и *
    EXPECT_EQ(UrlEncode("text+more"sv), "text%2Bmore"s);          // + кодируется как %2B
    EXPECT_EQ(UrlEncode("Sample&Value"sv), "Sample%26Value"s);    // & кодируется как %26
}