#include "MacGameBootstrap.h"

#include <radthread.hpp>
#include <radmemory.hpp>
#include <radtime.hpp>
#include <radfile.hpp>
#include <radload/radload.hpp>

// This target is intentionally not part of the shipped host yet.  It forces
// the linker to resolve the original game runtime while native services are
// ported incrementally.
int main()
{
    // The Cocoa host owns these services in the shipped application.  The
    // standalone probe must create the same foundation before game singletons
    // register their file and music loaders.
    radThreadInitialize();
    radMemoryInitialize();
    radTimeInitialize();
    radFileInitialize(50, 32, RADMEMORY_ALLOC_DEFAULT);
    radLoadInitialize();

    const bool initialized = MacGameBootstrapInitialize();
    MacGameBootstrapShutdown();

    radLoadTerminate();
    radFileTerminate();
    radTimeTerminate();
    radMemoryTerminate();
    radThreadTerminate();
    return initialized ? 0 : 1;
}
