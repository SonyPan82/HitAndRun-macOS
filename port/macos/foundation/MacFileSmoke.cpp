#include <radfile.hpp>
#include <radmemory.hpp>
#include <radthread.hpp>

#include <array>

int main()
{
    radThreadInitialize();
    radMemoryInitialize();
    radFileInitialize(8, 4);

    IRadFile* executable = nullptr;
    radFileOpenSync(&executable, "ROOT:Simpsons.exe");
    if (executable == nullptr || !executable->IsOpen() || executable->GetSize() == 0)
    {
        if (executable != nullptr) executable->Release();
        radFileTerminate();
        radMemoryTerminate();
        radThreadTerminate();
        return 1;
    }

    std::array<unsigned char, 2> signature{};
    executable->ReadSync(signature.data(), static_cast<unsigned int>(signature.size()));
    const bool validDosExecutable = signature[0] == 'M' && signature[1] == 'Z';
    executable->Release();

    IRadCementLibrary* scripts = nullptr;
    radFileRegisterCementLibrarySync(&scripts, "ROOT:scripts.rcf");
    if (scripts == nullptr || !scripts->IsOpen())
    {
        if (scripts != nullptr) scripts->Release();
        radFileTerminate();
        radMemoryTerminate();
        radThreadTerminate();
        return 3;
    }

    IRadFile* soundScript = nullptr;
    radFileOpenSync(&soundScript, "ROOT:sound\\scripts\\Apu.spt");
    const bool validArchivedScript = soundScript != nullptr && soundScript->IsOpen() && soundScript->GetSize() > 0;
    if (soundScript != nullptr) soundScript->Release();
    scripts->Release();

    radFileTerminate();
    radMemoryTerminate();
    radThreadTerminate();
    return validDosExecutable && validArchivedScript ? 0 : 2;
}
