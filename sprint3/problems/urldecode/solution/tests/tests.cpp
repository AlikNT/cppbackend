#define BOOST_TEST_MODULE urlencode tests
#include <boost/test/unit_test.hpp>

#include "../src/urldecode.h"

BOOST_AUTO_TEST_CASE(UrlDecode_tests) {
    using namespace std::literals;

   // Пустая строка
    BOOST_TEST(UrlDecode(""sv) == ""s);

    // Строка без %-последовательностей
    BOOST_TEST(UrlDecode("Hello World!"sv) == "Hello World!"s);

    // Строка с валидными %-последовательностями, записанными в разном регистре
    BOOST_TEST(UrlDecode("Hello%20World"sv) == "Hello World"s); // %20 - пробел
    BOOST_TEST(UrlDecode("Hello%2fWorld"sv) == "Hello/World"s); // %2f (нижний регистр) - /
    BOOST_TEST(UrlDecode("Hello%2FWorld"sv) == "Hello/World"s); // %2F (верхний регистр) - /

    // Строка с невалидными %-последовательностями
    BOOST_CHECK_THROW(UrlDecode("Hello%2GWorld"sv), std::invalid_argument); // %2G - недопустимая шестнадцатеричная цифра
    BOOST_CHECK_THROW(UrlDecode("Hello%ZZWorld"sv), std::invalid_argument); // %ZZ - оба символа недопустимы

    // Строка с неполными %-последовательностями
    BOOST_CHECK_THROW(UrlDecode("Hello%"sv), std::invalid_argument);       // % без символов
    BOOST_CHECK_THROW(UrlDecode("Hello%2"sv), std::invalid_argument);      // %2 - один символ после %

    // Строка с символом +
    BOOST_TEST(UrlDecode("Hello+World!"sv) == "Hello World!"s); // + заменяется на пробел
    BOOST_TEST(UrlDecode("C%2B%2B+language"sv) == "C++ language"s); // %2B - символ + в URL-кодировке

    // Дополнительные тесты для граничных случаев
    BOOST_TEST(UrlDecode("100%25"sv) == "100%"s); // %25 - символ %
}