#include <tuple>

#include <catch2/catch.hpp>
#include <glm/glm.hpp>

#include "kaacore/engine.h"
#include "kaacore/materials.h"
#include "kaacore/resources.h"
#include "kaacore/textures.h"
#include "runner.h"

kaacore::ResourceReference<kaacore::Texture>
_create_test_texture()
{
    const std::vector<uint8_t> image_content{0xFF, 0xFF, 0xFF, 0xFF};
    auto image_container = kaacore::load_raw_image(
        bimg::TextureFormat::Enum::RGBA8, 1, 1, image_content);
    return kaacore::MemoryTexture::create(image_container);
}

TEST_CASE("Test materials")
{
    SECTION("basic use")
    {
        auto engine = initialize_testing_engine();
        auto texture = _create_test_texture();
        auto program = engine->renderer->default_material->program;
        auto material = kaacore::Material::create(
            program,
            {
                {"sampler",
                 kaacore::UniformSpecification(kaacore::UniformType::sampler)},
                {"vector",
                 kaacore::UniformSpecification(kaacore::UniformType::vec4)},
                {"vector2",
                 kaacore::UniformSpecification(kaacore::UniformType::vec4, 2)},
            });
        glm::fvec4 vector({1.f, 1.f, 0, 0});
        glm::fvec4 vector2({1.f, 1.f, 1.f, 0});

        material->set_uniform_texture("sampler", texture, 11, 11);
        material->set_uniform_value<glm::vec4>("vector", vector);
        material->set_uniform_value<glm::vec4>(
            "vector2", std::vector<glm::vec4>({vector, vector2}));

        // sampler
        kaacore::SamplerValue sampler_value =
            material->get_uniform_texture("sampler").value();
        REQUIRE(sampler_value.stage == 11);
        REQUIRE(sampler_value.flags == 11);
        REQUIRE(sampler_value.texture == texture);

        // vector
        auto result = material->get_uniform_value<glm::vec4>("vector");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0] == vector);

        // vector_v2
        auto result_v2 = material->get_uniform_value<glm::vec4>("vector2");
        REQUIRE(result_v2.size() == 2);
        REQUIRE(result_v2[0] == vector);
        REQUIRE(result_v2[1] == vector2);
    }

    SECTION("cloning")
    {
        auto engine = initialize_testing_engine();
        auto texture = _create_test_texture();
        auto program = engine->renderer->default_material->program;
        auto material = kaacore::Material::create(
            program, {{"sampler", kaacore::UniformSpecification(
                                      kaacore::UniformType::sampler)}});

        material->set_uniform_texture("sampler", texture, 11, 11);
        REQUIRE(material->get_uniform_texture("sampler").value().stage == 11);
        REQUIRE(material->get_uniform_texture("sampler").value().flags == 11);
        REQUIRE(
            material->get_uniform_texture("sampler").value().texture ==
            texture);

        auto material2 = material->clone();
        REQUIRE(material2->get_uniform_texture("sampler").value().stage == 11);
        REQUIRE(material2->get_uniform_texture("sampler").value().flags == 11);
        REQUIRE(
            material2->get_uniform_texture("sampler").value().texture ==
            texture);

        material2->set_uniform_texture("sampler", texture, 12, 12);
        REQUIRE(material2->get_uniform_texture("sampler").value().stage == 12);
        REQUIRE(material2->get_uniform_texture("sampler").value().flags == 12);
        REQUIRE(
            material2->get_uniform_texture("sampler").value().texture ==
            texture);

        REQUIRE(material->get_uniform_texture("sampler").value().stage == 11);
        REQUIRE(material->get_uniform_texture("sampler").value().flags == 11);
        REQUIRE(
            material->get_uniform_texture("sampler").value().texture ==
            texture);
    }

    SECTION("errors")
    {
        auto engine = initialize_testing_engine();
        auto texture = _create_test_texture();
        auto program = engine->renderer->default_material->program;
        auto material = kaacore::Material::create(program);
        glm::fvec4 vector({1.f, 1.f, 0, 0});
        glm::fvec4 vector2({1.f, 1.f, 1.f, 0});

        // nonexistent uniform
        REQUIRE_THROWS(material->set_uniform_texture("missing", texture, 1));
        REQUIRE_THROWS(material->get_uniform_texture("missing"));

        material = kaacore::Material::create(
            program,
            {{"sampler",
              kaacore::UniformSpecification(kaacore::UniformType::sampler)},
             {"vector",
              kaacore::UniformSpecification(kaacore::UniformType::vec4)}});

        // stage 0 is reserved for internal use
        REQUIRE_THROWS(material->set_uniform_texture("sampler", texture, 0));

        // invalid number of elements
        REQUIRE_THROWS(material->set_uniform_value<glm::fvec4>(
            "vector", std::vector<glm::fvec4>({vector, vector2})));

        // reserved uniform name
        REQUIRE_THROWS(kaacore::Material::create(
            program, {{"s_texture", kaacore::UniformSpecification(
                                        kaacore::UniformType::sampler)}}));
    }
}
