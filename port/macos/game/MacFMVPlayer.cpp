#include <presentation/fmvplayer/fmvplayer.h>
#include <presentation/fmvplayer/fmvuserinputhandler.h>
#include <input/inputmanager.h>
#include <port/macos/foundation/MacGameData.h>

#include "MacMovieOverlay.h"

#include <algorithm>
#include <filesystem>

namespace
{
std::string NativeMoviePath(const char* fileName)
{
    if (fileName == nullptr) return {};
    std::string name(fileName);
    std::replace(name.begin(), name.end(), '\\', '/');
    std::filesystem::path path(name);
    if (!path.is_absolute()) path = std::filesystem::path(MacGameDataRoot()) / path;
    path.replace_extension(".mp4");
    return path.string();
}
}

// PC .rmv files are Bink streams. At packaging time they are transcoded to
// H.264 MP4; AVFoundation then decodes them natively on Apple Silicon.
FMVPlayer::FMVPlayer()
    : m_UserInputHandler(nullptr), mFrameReady(false), mElapsedTime(0.0f),
      mDriveFinished(true), mFadeOut(-1.0f), mMovieVolume(1.0f),
      mNativeMovieStarted(false)
{
    m_UserInputHandler = new FMVUserInputHandler;
    m_UserInputHandler->AddRef();
    SetState(ANIM_IDLE);
}

FMVPlayer::~FMVPlayer()
{
    MacMovieOverlayStop();
    if (m_UserInputHandler != nullptr) m_UserInputHandler->Release();
}

void FMVPlayer::Play()
{
    if (GetState() != ANIM_LOADED || mNativeMoviePath.empty()) return;
    AnimationPlayer::Play();
    mElapsedTime = 0.0f;
    mNativeMovieStarted = MacMovieOverlayPlay(mNativeMoviePath.c_str(), mMovieVolume);
    if (!mNativeMovieStarted) { Stop(); return; }
    for (unsigned i = 0; i < GetInputManager()->GetMaxControllers(); ++i)
        GetInputManager()->RegisterMappable(i, m_UserInputHandler);
}

void FMVPlayer::Abort() { Stop(); }

void FMVPlayer::Stop()
{
    MacMovieOverlayStop();
    for (unsigned i = 0; mNativeMovieStarted && i < GetInputManager()->GetMaxControllers(); ++i)
        GetInputManager()->UnregisterMappable(i, m_UserInputHandler);
    mNativeMovieStarted = false;
    AnimationPlayer::Stop();
}

void FMVPlayer::Pause() { MacMovieOverlayPause(); }
void FMVPlayer::UnPause() { MacMovieOverlayUnpause(); }

void FMVPlayer::LoadData(const char* fileName, bool, void*)
{
    mNativeMoviePath = NativeMoviePath(fileName);
    SetState(std::filesystem::exists(mNativeMoviePath) ? ANIM_LOADED : ANIM_STOPPED);
}

void FMVPlayer::IterateLoop(IRadMoviePlayer2*) { mFrameReady = false; }

void FMVPlayer::ClearData()
{
    MacMovieOverlayStop();
    m_refIRadMoviePlayer = nullptr;
    mNativeMoviePath.clear();
    mNativeMovieStarted = false;
    mElapsedTime = 0.0f;
    mFrameReady = false;
    SetState(ANIM_IDLE);
}

void FMVPlayer::Initialize(radMemoryAllocator) {}

void FMVPlayer::DoRender()
{
    mElapsedTime = MacMovieOverlayElapsedSeconds();
    if (!MacMovieOverlayIsPlaying()) Stop();
}

void FMVPlayer::OnDriveOperationsComplete(void*) { mDriveFinished = true; }
void FMVPlayer::FadeScreen(float alpha) { mFadeOut = alpha; }
