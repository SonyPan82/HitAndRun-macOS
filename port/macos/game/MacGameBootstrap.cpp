#include "MacGameBootstrap.h"
#include "MacGamePlatform.h"

#include <main/game.h>
#include <main/commandlineoptions.h>
#include <main/singletons.h>
#include <gameflow/gameflow.h>
#include <memory/memoryutilities.h>
#include <memory/srrmemory.h>
#include <radtime.hpp>

namespace
{
MacGamePlatform* gPlatform = nullptr;
Game* gGame = nullptr;
bool gInitialized = false;
bool gSingletonsCreated = false;
int gLastReportedContext = -1;
unsigned int gTickCount = 0;
}

bool MacGameBootstrapCreate()
{
    if (gGame != nullptr)
        return true;
    gPlatform = new MacGamePlatform();
    gGame = Game::CreateInstance(gPlatform);
    return gGame != nullptr;
}

bool MacGameBootstrapInitialize()
{
    if (!MacGameBootstrapCreate())
        return false;
    if (!gInitialized)
    {
        CommandLineOptions::InitDefaults();
        Memory::InitializeMemoryUtilities();
        HeapMgr()->PrepareHeapsStartup();
        HeapMgr()->PushHeap(GMA_PERSISTENT);
        CreateSingletons();
        gSingletonsCreated = true;
        gGame->Initialize();
        HeapMgr()->PopHeap(GMA_PERSISTENT);
        gInitialized = true;
    }
    return true;
}

void MacGameBootstrapTick()
{
    if (gInitialized && gGame != nullptr)
    {
        gGame->Run();
        ++gTickCount;
        if ((gTickCount % 300u) == 0u)
            rReleasePrintf("macOS game ticks: %u\n", gTickCount);
        const int context = static_cast<int>(GetGameFlow()->GetCurrentContext());
        if (context != gLastReportedContext)
        {
            rReleasePrintf("macOS game context: %d\n", context);
            gLastReportedContext = context;
        }
    }
}

void MacGameBootstrapShutdown()
{
    if (gGame == nullptr)
        return;
    if (gInitialized)
    {
        gGame->Stop();
        gGame->Terminate();
        gInitialized = false;
        gLastReportedContext = -1;
        gTickCount = 0;
    }
    if (gSingletonsCreated)
    {
        DestroySingletons();
        gSingletonsCreated = false;
    }
    Game::DestroyInstance();
    gGame = nullptr;
    delete gPlatform;
    gPlatform = nullptr;
}
