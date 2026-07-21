#include <raddebugcommunication.hpp>
#include <raddebugconsole.hpp>
#include <radremotecommand.hpp>
#include <radtextdisplay.hpp>

namespace
{
class MacRemoteCommand final : public IRadRemoteCommand
{
public:
    void AddRef() override { ++mReferences; }
    void Release() override { if (mReferences > 1) --mReferences; }
    void RegisterRemoteFunction(char*, RemoteFunction, void*) override {}
    void UnRegisterRemoteFunction(char*) override {}
private:
    unsigned mReferences = 1;
};

class MacTextDisplay final : public IRadTextDisplay
{
public:
    void AddRef() override { ++mReferences; }
    void Release() override { if (mReferences > 1) --mReferences; }
    void SetAutoSwap(bool) override {}
    void SwapBuffers() override {}
    void SetBackgroundColor(unsigned int) override {}
    void SetTextColor(unsigned int) override {}
    void Clear() override {}
    void GetDimensions(unsigned int* width, unsigned int* height) const override
    {
        if (width) *width = 0;
        if (height) *height = 0;
    }
    void SetCursorPosition(unsigned int, unsigned int) override {}
    void TextOutAt(const char*, int, int) override {}
    void TextOut(const char*) override {}
private:
    unsigned mReferences = 1;
};

MacRemoteCommand gRemoteCommand;
MacTextDisplay gTextDisplay;
}

void radDbgComService() {}
void radDebugConsoleService() {}
void radRemoteCommandInitialize(radMemoryAllocator) {}
void radRemoteCommandTerminate() {}
IRadRemoteCommand* radRemoteCommandGet() { return &gRemoteCommand; }
void radTextDisplayGet(IRadTextDisplay** display, radMemoryAllocator)
{
    if (display != nullptr) { gTextDisplay.AddRef(); *display = &gTextDisplay; }
}
