#include <catch2/catch.hpp>
#include <glm/glm.hpp>

#include "kaacore/geometry.h"
#include "kaacore/shapes.h"

TEST_CASE("Test circle transformation", "[shapes][no_engine]")
{
    auto circle_shape = kaacore::Shape::Circle(10.);
    auto complex_transformation =
        kaacore::Transformation::translate({20., 20.}) |
        kaacore::Transformation::rotate(2.05) |
        kaacore::Transformation::scale({5., 5.}) |
        kaacore::Transformation::translate({30., -50.});

    SECTION("Correct case")
    {
        REQUIRE_NOTHROW(circle_shape.transform(complex_transformation));
    }

    SECTION("Scaling by non-equal vector")
    {
        auto non_equal_scale_transformation =
            complex_transformation | kaacore::Transformation::scale({0.5, 1.});
        REQUIRE_THROWS_WITH(
            circle_shape.transform(non_equal_scale_transformation),
            "Cannot transform shape radius by non-equal scale");
    }
}
