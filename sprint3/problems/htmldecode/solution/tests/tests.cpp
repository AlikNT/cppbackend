#include <catch2/catch_test_macros.hpp>

#include "../src/htmldecode.h"

using namespace std::literals;

TEST_CASE("Text without mnemonics", "[HtmlDecode]") {
    CHECK(HtmlDecode(""sv) == ""s);
    CHECK(HtmlDecode("hello"sv) == "hello"s);
}

TEST_CASE("Basic HTML entities decoding") {
    REQUIRE(HtmlDecode("M&amp;M&apos;s") == "M&M's");
    REQUIRE(HtmlDecode("Johnson&amp;Johnson") == "Johnson&Johnson");
    REQUIRE(HtmlDecode("2 &lt; 3 &gt; 1") == "2 < 3 > 1");
    REQUIRE(HtmlDecode("Hello &quot;world&quot;") == "Hello \"world\"");
}

TEST_CASE("Entities without semicolons") {
    REQUIRE(HtmlDecode("M&AMPM&APOSs") == "M&M's");
    REQUIRE(HtmlDecode("Johnson&ampJohnson") == "Johnson&Johnson");
}

TEST_CASE("Unknown entities") {
    REQUIRE(HtmlDecode("Random &abracadabra test") == "Random &abracadabra test");
}

TEST_CASE("Mixed case entities") {
    REQUIRE(HtmlDecode("M&AMP;M&APOS;s") == "M&M's");
    REQUIRE(HtmlDecode("&AMP;lt;") == "&lt;");
}

TEST_CASE("Entities with special cases") {
    REQUIRE(HtmlDecode("&amp;lt;") == "&lt;");
    REQUIRE(HtmlDecode("&abracadabra"sv) == "&abracadabra"s);
}
