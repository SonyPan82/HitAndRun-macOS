#pragma once

// The original Pure3D runtime TTF generator is absent from this source drop.
// Scrooby still references its small public façade, so retain that façade on
// macOS.  Normal game fonts are texture fonts loaded from the PC P3D assets.
#include <p3d/file.hpp>
#include <p3d/texturefont.hpp>

#ifndef P3D_WCHAR
#define P3D_WCHAR P3D_UNICODE
#endif

enum p3dLegacyFontTarget
{
    P3D_SCREEN_FONT = 0
};

class tTypeFace
{
public:
    tTypeFace(tFileMem*, int) {}
    void AddRef() {}
    void Release() { delete this; }
    void SetCharacters(const char*) {}
    void SetCharacters(const P3D_UNICODE*) {}
    tTextureFont* MakeTextureFont(const char*, p3dLegacyFontTarget, int size)
    {
        return tTextureFont::CreateLegacyFallback(static_cast<float>(size));
    }

private:
    ~tTypeFace() {}
};
