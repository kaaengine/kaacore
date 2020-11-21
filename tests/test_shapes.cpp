#include <catch2/catch.hpp>
#include <glm/glm.hpp>

#include "kaacore/geometry.h"
#include "kaacore/shapes.h"

using namespace kaacore;

TEST_CASE("Test circle transformation", "[shapes][no_engine]")
{
    auto circle_shape = Shape::Circle(10.);
    auto complex_transformation = Transformation::translate({20., 20.}) |
                                  Transformation::rotate(2.05) |
                                  Transformation::scale({5., 5.}) |
                                  Transformation::translate({30., -50.});

    SECTION("Correct case")
    {
        REQUIRE_NOTHROW(circle_shape.transform(complex_transformation));
    }

    SECTION("Scaling by non-equal vector")
    {
        auto non_equal_scale_transformation =
            complex_transformation | Transformation::scale({0.5, 1.});
        REQUIRE_THROWS_WITH(
            circle_shape.transform(non_equal_scale_transformation),
            "Cannot transform shape radius by non-equal scale");
    }
}
