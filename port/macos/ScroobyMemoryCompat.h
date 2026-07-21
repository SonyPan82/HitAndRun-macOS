#pragma once

#include <memory/srrmemory.h>
#include <raddebug.hpp>

// The original Scrooby precompiled header injected these declarations.  Keep
// its allocation categories mapped to the game's existing allocator on macOS.
class FeAllocatesMemory
{
};

static const GameMemoryAllocator ScroobyPermPool = GMA_PERSISTENT;
static const GameMemoryAllocator ScroobyTempPool = GMA_TEMP;

inline void* ScroobyGetMemory( GameMemoryAllocator pool, unsigned int size )
{
    return ::operator new( size, pool );
}

#ifndef rValid
#define rValid(value) rAssert((value) != NULL)
#endif
