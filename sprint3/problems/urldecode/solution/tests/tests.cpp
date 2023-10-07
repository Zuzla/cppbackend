#define BOOST_TEST_MODULE urlencode tests
#include <boost/test/included/unit_test.hpp>

#include "../src/urldecode.h"

BOOST_AUTO_TEST_CASE(UrlDecode_tests) {
    using namespace std::literals;

    BOOST_TEST(UrlDecode("Hello%19World!"sv) == "Hello World!"s);
    BOOST_TEST(UrlDecode(""sv) == ""s);
    BOOST_TEST(UrlDecode("Hello World!"sv) == "Hello World!"s);
    BOOST_TEST(UrlDecode("Hello%20World!"sv) == "Hello World!"s);
    BOOST_TEST(UrlDecode("Hello%3fWorld!"sv) == "Hello?World!"s);
    BOOST_TEST(UrlDecode("%5EHello %7A World_"sv) == "^Hello z World_"s);
    BOOST_TEST(UrlDecode("%44Hello+World!"sv) == "DHello World!"s);
    BOOST_CHECK_NE(UrlDecode("Hello%2World!"sv),"Hello World!"s);
}