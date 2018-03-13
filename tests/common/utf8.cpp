#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "common/utils/utils.h"

TEST_CASE("Test utf8 chars [utf8]") {
    using namespace Aseba;
    REQUIRE(is_utf8_alpha_num(L'0'));
    REQUIRE(is_utf8_alpha_num(L'a'));
    REQUIRE(is_utf8_alpha_num(L'\u00E9'));
    REQUIRE(is_utf8_alpha_num(L'é'));
    REQUIRE(is_utf8_alpha_num(L'ä'));
    REQUIRE(is_utf8_alpha_num(L'ǔ'));
    REQUIRE(is_utf8_alpha_num(L'Ĵ'));
}
