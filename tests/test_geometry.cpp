#include <cmath>

#include <catch2/catch.hpp>

#include "kaacore/geometry.h"

#include "runner.h"

TEST_CASE(
    "test_normalize_angle_degrees", "[geometry][normalize_angle][no_engine]")
{
    using kaacore::AngleSign;
    using kaacore::normalize_angle_degrees;

    REQUIRE(normalize_angle_degrees(30., AngleSign::mixed) == 30.);
    REQUIRE(normalize_angle_degrees(-30., AngleSign::mixed) == -30.);
    REQUIRE(normalize_angle_degrees(-180., AngleSign::mixed) == -180.);
    REQUIRE(normalize_angle_degrees(180., AngleSign::mixed) == -180.);
    REQUIRE(normalize_angle_degrees(0., AngleSign::mixed) == 0.);
    REQUIRE(normalize_angle_degrees(360., AngleSign::mixed) == 0.);

    REQUIRE(normalize_angle_degrees(30., AngleSign::positive) == 30.);
    REQUIRE(normalize_angle_degrees(-30., AngleSign::positive) == 330.);
    REQUIRE(normalize_angle_degrees(-180., AngleSign::positive) == 180.);
    REQUIRE(normalize_angle_degrees(180., AngleSign::positive) == 180.);
    REQUIRE(normalize_angle_degrees(0., AngleSign::positive) == 0.);
    REQUIRE(normalize_angle_degrees(360., AngleSign::positive) == 0.);

    REQUIRE(normalize_angle_degrees(30., AngleSign::negative) == -330.);
    REQUIRE(normalize_angle_degrees(-30., AngleSign::negative) == -30.);
    REQUIRE(normalize_angle_degrees(-180., AngleSign::negative) == -180.);
    REQUIRE(normalize_angle_degrees(180., AngleSign::negative) == -180.);
    REQUIRE(normalize_angle_degrees(0., AngleSign::negative) == 0.);
    REQUIRE(normalize_angle_degrees(-360., AngleSign::negative) == 0.);
}

TEST_CASE("test_normalize_angle", "[geometry][normalize_angle][no_engine]")
{
    using kaacore::AngleSign;
    using kaacore::normalize_angle;

    REQUIRE(normalize_angle(M_PI / 4., AngleSign::mixed) == M_PI / 4.);
    REQUIRE(normalize_angle(-M_PI / 4., AngleSign::mixed) == -M_PI / 4.);
    REQUIRE(normalize_angle(-M_PI, AngleSign::mixed) == -M_PI);
    REQUIRE(normalize_angle(M_PI, AngleSign::mixed) == -M_PI);
    REQUIRE(normalize_angle(0., AngleSign::mixed) == 0.);
    REQUIRE(normalize_angle(2 * M_PI, AngleSign::mixed) == 0.);

    REQUIRE(normalize_angle(M_PI / 4., AngleSign::positive) == M_PI / 4.);
    REQUIRE(normalize_angle(-M_PI / 4., AngleSign::positive) == 7 * M_PI / 4.);
    REQUIRE(normalize_angle(-M_PI, AngleSign::positive) == M_PI);
    REQUIRE(normalize_angle(M_PI, AngleSign::positive) == M_PI);
    REQUIRE(normalize_angle(0., AngleSign::positive) == 0.);
    REQUIRE(normalize_angle(2 * M_PI, AngleSign::positive) == 0.);
}
