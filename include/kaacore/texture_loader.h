#pragma once

#include <bgfx/bgfx.h>


bgfx::TextureHandle load_texture(const uint8_t* data, size_t size,
    uint64_t flags=BGFX_SAMPLER_NONE);
bgfx::TextureHandle load_texture_from_file(const char* path,
        uint64_t flags=BGFX_SAMPLER_NONE);
