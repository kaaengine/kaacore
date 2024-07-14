#pragma once

#include <utility>
#include <vector>

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

namespace kaacore {

struct StandardVertexData;

using VertexIndex = uint16_t;
using VerticesIndicesVectorPair =
    std::pair<std::vector<StandardVertexData>, std::vector<VertexIndex>>;

struct StandardVertexData {
    glm::fvec3 xyz;
    glm::fvec2 uv;
    glm::fvec2 mn;
    glm::fvec4 rgba;

    static bgfx::VertexLayout init()
    {
        bgfx::VertexLayout vertex_layout;
        vertex_layout.begin()
            .add(bgfx::Attrib::Enum::Position, 3, bgfx::AttribType::Enum::Float)
            .add(
                bgfx::Attrib::Enum::TexCoord0, 2, bgfx::AttribType::Enum::Float
            )
            .add(
                bgfx::Attrib::Enum::TexCoord1, 2, bgfx::AttribType::Enum::Float
            )
            .add(bgfx::Attrib::Enum::Color0, 4, bgfx::AttribType::Enum::Float)
            .end();
        return vertex_layout;
    };

    StandardVertexData(
        float x = 0., float y = 0., float z = 0., float u = 0., float v = 0.,
        float m = 0., float n = 0., float r = 1., float g = 1., float b = 1.,
        float a = 1.
    )
        : xyz(x, y, z), uv(u, v), mn(m, n), rgba(r, g, b, a){};

    static inline StandardVertexData xy_uv(float x, float y, float u, float v)
    {
        return StandardVertexData(x, y, 0., u, v);
    }

    static inline StandardVertexData xy_uv_mn(
        float x, float y, float u, float v, float m, float n
    )
    {
        return StandardVertexData(x, y, 0., u, v, m, n);
    }

    inline bool operator==(const StandardVertexData& other) const
    {
        return (
            this->xyz == other.xyz and this->uv == other.uv and
            this->mn == other.mn and this->rgba == other.rgba
        );
    }
};
} // namespace kaacore
