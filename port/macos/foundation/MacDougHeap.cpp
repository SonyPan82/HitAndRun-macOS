#include <radmemory.hpp>

// The original PC allocator embeds a 32-bit Doug Lea implementation whose
// address bookkeeping cannot represent arm64 pointers.  TrackingHeap already
// provides bounded, freeable allocator semantics required by the game's Doug
// Lea heaps, so use it as the native arm64 implementation.
IRadMemoryHeap* radMemoryCreateDougLeaHeap(unsigned int size,
                                           radMemoryAllocator allocator,
                                           const char* name)
{
    return radMemoryCreateTrackingHeap(size, allocator, name);
}

IRadMemoryHeap* radMemoryCreateDougLeaHeap(void* memory,
                                           unsigned int size,
                                           radMemoryAllocator allocator,
                                           const char* name)
{
    (void)memory;
    return radMemoryCreateTrackingHeap(size, allocator, name);
}
