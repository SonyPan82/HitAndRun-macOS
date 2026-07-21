#include <cmath>
#include <thread>
#include <chrono>

#include <radmemory.hpp>
#include <radthread.hpp>
#include <radsound_hal.hpp>

int main()
{
    radThreadInitialize();
    radMemoryInitialize();
    radSoundHalSystemInitialize(RADMEMORY_ALLOC_DEFAULT);

    IRadSoundHalSystem::SystemDescription description = {};
    description.m_MaxRootAllocations = 16;
    description.m_NumAuxSends = 0;
    description.m_ReservedSoundMemory = 256 * 1024;
    IRadSoundHalSystem* system = radSoundHalSystemGet();
    if (system == nullptr) return 1;
    system->Initialize(description);

    IRadSoundHalAudioFormat* format = radSoundHalAudioFormatCreate(RADMEMORY_ALLOC_DEFAULT);
    format->AddRef();
    format->Initialize(IRadSoundHalAudioFormat::PCM, nullptr, 48000, 1, 16);
    const unsigned frames = 4800;
    IRadMemoryObject* memory = nullptr;
    system->GetRootMemoryRegion()->CreateMemoryObject(&memory, format->FramesToBytes(frames), "audio-smoke");
    if (memory == nullptr) return 2;
    memory->AddRef();
    auto* samples = static_cast<short*>(memory->GetMemoryAddress());
    for (unsigned i = 0; i < frames; ++i)
        samples[i] = static_cast<short>(std::sin(i * 2.0 * 3.141592653589793 * 440.0 / 48000.0) * 4096.0);

    IRadSoundHalBuffer* buffer = radSoundHalBufferCreate(RADMEMORY_ALLOC_DEFAULT);
    buffer->AddRef();
    buffer->Initialize(format, memory, frames, false, false);
    IRadSoundHalVoice* voice = radSoundHalVoiceCreate(RADMEMORY_ALLOC_DEFAULT);
    voice->AddRef();
    voice->SetBuffer(buffer);
    voice->SetVolume(0.05f);
    voice->Play();
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    const bool advanced = voice->GetPlaybackPositionInSamples() > 0;
    voice->Stop();
    voice->Release();
    buffer->Release();
    memory->Release();
    format->Release();
    radSoundHalSystemTerminate();
    radMemoryTerminate();
    radThreadTerminate();
    return advanced ? 0 : 3;
}
