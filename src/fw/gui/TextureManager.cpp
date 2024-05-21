#include "TextureManager.h"

#include <gl3w/GL/gl3w.h>
#include <gl3w/GL/glcorearb.h>
#include <lodepng/lodepng.h>
#include <map>
#include <string>
#include <vector>

#include "fw/core/log.h"
#include "Texture.h"

using namespace fw;

Texture* TextureManager::load(const std::string& path)
{
    // Return if already exists
    auto tex = m_register.find(path);
    if (tex != m_register.end() )
        return tex->second;

    return load_png_to_gpu(path);
}

bool TextureManager::release_all()
{
    bool success = true;
    for( const auto& [key, texture] : m_register )
    {
        if( texture->gl_handler ) // is zero when texture is not loaded to GPU
        {
            glDeleteTextures(1, &texture->gl_handler);
            if( GL_NO_ERROR != glGetError())
            {
                success = false;
                LOG_WARNING("TextureManager", "Unable to release: %s (code: %i)\n", key.c_str(), glGetError())
            }
            else
            {
                LOG_MESSAGE("TextureManager", "Released %s\n", key.c_str())
            }
        }
        delete texture;
    }
    m_register.clear();
    return success;
}
Texture *TextureManager::load_png_to_gpu(const std::string &path)
{
    auto* texture = new Texture();

    // 1. Load png file to Texture (RAM only)
    int error = load_png(path, texture);
    if ( error )
    {
        delete texture;
        LOG_ERROR("TextureManager", "Unable to load png (code %u): %s\n",  error, path.c_str())
        return nullptr;
    }

    // 2. Load texture to GPU
    error = load_to_gpu(texture);
    if ( error )
    {
        delete texture;
        LOG_ERROR("TextureManager", "Unable to load texture to GPU (code %u): %s\n",  error, path.c_str())
        return nullptr;
    }

    m_register.insert({path, texture});
    LOG_MESSAGE("TextureManager", "File loaded to GPU: %s\n", path.c_str())

    return texture;
}

int TextureManager::load_png(const std::string &filename, Texture* texture)
{
    LOG_MESSAGE("TextureManager", "Loading PNG from disk %s ...\n", filename.c_str());
    std::vector<unsigned char> buffer;
    unsigned error = lodepng::load_file(buffer, filename); //load the image file with given filename
    if (error) {
        LOG_MESSAGE("TextureManager", "Error: %i %s\n", error, lodepng_error_text(error) );
        return 1;
    }
    LOG_MESSAGE("TextureManager", "Decoding PNG %s ...\n", filename.c_str());
    error = lodepng::decode(texture->buffer, (unsigned&)texture->width, (unsigned&)texture->height, buffer); //decode the png
    if (error) {
        LOG_MESSAGE("TextureManager", "Error: %i %s\n", error, lodepng_error_text(error) );
        return 2;
    }
    LOG_MESSAGE("TextureManager", "PNG read (image: %i x %i px)\n", texture->width, texture->height );
    return 0;
}

int TextureManager::load_to_gpu(Texture* texture)
{
    // Create a OpenGL texture identifier
    glGenTextures(1, &texture->gl_handler);
    glBindTexture(GL_TEXTURE_2D, texture->gl_handler);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Load image data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)texture->width, (int)texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->buffer.data());

    return glGetError();
}
