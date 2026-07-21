#pragma once

#include <main/platform.h>

// Native counterpart of Win32Platform.  Cocoa owns the window and GL context;
// this class owns the game-facing filesystem and input lifecycle.
class MacGamePlatform final : public Platform
{
public:
    MacGamePlatform();
    ~MacGamePlatform();

    void InitializePlatform() override;
    void ShutdownPlatform() override;
    void LaunchDashboard() override;
    void ResetMachine() override;
    void DisplaySplashScreen(SplashScreen screenID, const char* overlayText = NULL,
        float fontScale = 1.0f, float textPosX = 0.0f, float textPosY = 0.0f,
        tColour textColour = tColour(255, 255, 255), int fadeFrames = 3) override;
    void DisplaySplashScreen(const char* textureName, const char* overlayText = NULL,
        float fontScale = 1.0f, float textPosX = 0.0f, float textPosY = 0.0f,
        tColour textColour = tColour(255, 255, 255), int fadeFrames = 3) override;
    void OnControllerError(const char* msg) override;
    bool OnDriveError(radFileError error, const char* driveName, void* userData) override;

protected:
    void InitializeFoundationDrive() override;
    void ShutdownFoundation() override;
    void InitializePure3D() override;
    void ShutdownPure3D() override;

private:
    bool mInitialized;
};
