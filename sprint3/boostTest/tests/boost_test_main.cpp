#define BOOST_TEST_MODULE leap year application tests
#include <boost/test/included/unit_test.hpp>

#include "../src/leap_year.h"

BOOST_AUTO_TEST_CASE(IsLeapYear_test) 
{
    BOOST_CHECK(IsLeapYear(2020));
    BOOST_CHECK(!IsLeapYear(2021));
    BOOST_CHECK(!IsLeapYear(2022));
    BOOST_CHECK(!IsLeapYear(2023));
    BOOST_CHECK(IsLeapYear(2024));
    BOOST_CHECK(!IsLeapYear(1900));
    BOOST_CHECK(IsLeapYear(2000));
}

int Sqr(int x) {
    return x * x;
}

BOOST_AUTO_TEST_CASE(Sqr_test) {
    BOOST_CHECK_EQUAL(Sqr(3), 9);
    BOOST_CHECK_EQUAL(Sqr(-5), 25);
} 


std::vector<int> GetEvenNumbers(const std::vector<int> numbers) {
    std::vector<int> even_numbers;
    even_numbers.reserve(numbers.size());
    std::copy_if(                          //
        numbers.begin(), numbers.end(),    //
        std::back_inserter(even_numbers),  //
        [](int number) {
            return number % 2 == 0;
        });
    return even_numbers;
}

BOOST_AUTO_TEST_CASE(GetEvenNumbers_test) {
    auto evens = GetEvenNumbers({1, 2, 3, 4, 5});
    BOOST_REQUIRE_EQUAL(evens.size(), 2);
    // Если предыдущая проверка провалится, то тест прервётся
    // и обращения к несуществующим элементам вектора не произойдёт

    BOOST_CHECK_EQUAL(evens[0], 2);
    BOOST_CHECK_EQUAL(evens[1], 4);
} 