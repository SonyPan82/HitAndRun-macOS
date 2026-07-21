#include <radthread.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace
{
class RefCounted
{
public:
    void AddReference() { ++mReferences; }
    bool RemoveReference() { return --mReferences == 0; }
private:
    std::atomic_int mReferences { 1 };
};

class MacMutex final : public IRadThreadMutex, private RefCounted
{
public:
    void AddRef() override { AddReference(); }
    void Release() override { if (RemoveReference()) delete this; }
    void Lock() override { mMutex.lock(); }
    void Unlock() override { mMutex.unlock(); }
private:
    std::recursive_mutex mMutex;
};

class MacSemaphore final : public IRadThreadSemaphore, private RefCounted
{
public:
    explicit MacSemaphore(unsigned int count) : mCount(count) {}
    void AddRef() override { AddReference(); }
    void Release() override { if (RemoveReference()) delete this; }
    void Wait() override { std::unique_lock<std::mutex> lock(mMutex); mCondition.wait(lock, [this] { return mCount != 0; }); --mCount; }
    void Signal() override { { std::lock_guard<std::mutex> lock(mMutex); ++mCount; } mCondition.notify_one(); }
private:
    std::mutex mMutex;
    std::condition_variable mCondition;
    unsigned int mCount;
};

class MacThread final : public IRadThread, private RefCounted
{
public:
    MacThread() : mRunning(false), mPriority(PriorityNormal) {}
    MacThread(RADTHREADENTRY entry, void* userData, Priority priority) : mRunning(true), mPriority(priority)
    {
        mThread = std::thread([this, entry, userData] { ActiveThread() = this; mResult = entry(userData); mRunning = false; mDone.notify_all(); });
    }
    ~MacThread() { if (mThread.joinable()) mThread.join(); }
    void AddRef() override { AddReference(); }
    void Release() override { if (RemoveReference()) delete this; }
    void SetPriority(Priority priority) override { mPriority = priority; }
    Priority GetPriority() override { return mPriority; }
    void Suspend() override { }
    void Resume() override { }
    bool IsRunning(unsigned int* result) override { if (!mRunning && result != nullptr) *result = mResult; return mRunning; }
    unsigned int WaitForTermination() override
    {
        if (mThread.joinable()) mThread.join();
        return mResult;
    }
    static MacThread*& ActiveThread() { static thread_local MacThread* thread = nullptr; return thread; }
private:
    std::thread mThread;
    std::atomic_bool mRunning;
    Priority mPriority;
    unsigned int mResult = 0;
    std::condition_variable mDone;
};

class MacThreadLocalStorage final : public IRadThreadLocalStorage, private RefCounted
{
public:
    void AddRef() override { AddReference(); }
    void Release() override { if (RemoveReference()) delete this; }
    void* GetValue() override { return Values()[this]; }
    void SetValue(void* value) override { Values()[this] = value; }
private:
    static std::unordered_map<const MacThreadLocalStorage*, void*>& Values() { static thread_local std::unordered_map<const MacThreadLocalStorage*, void*> values; return values; }
};

class MacFiber final : public IRadThreadFiber, private RefCounted
{
public:
    MacFiber(RADFIBERENTRY entry, void* value) : mEntry(entry), mValue(value) {}
    void AddRef() override { AddReference(); }
    void Release() override { if (RemoveReference()) delete this; }
    void SwitchTo() override { if (!mHasRun && mEntry != nullptr) { mHasRun = true; mEntry(mValue); } }
    void* GetValue() override { return mValue; }
    void SetValue(void* value) override { mValue = value; }
private:
    RADFIBERENTRY mEntry;
    void* mValue;
    bool mHasRun = false;
};

bool gThreadsInitialized = false;
MacThread* gMainThread = nullptr;
MacFiber* gMainFiber = nullptr;
}

void radThreadInitialize(unsigned int)
{
    if (gThreadsInitialized) return;
    gMainThread = new MacThread();
    MacThread::ActiveThread() = gMainThread;
    gMainFiber = new MacFiber(nullptr, nullptr);
    gThreadsInitialized = true;
}
void radThreadTerminate(void)
{
    if (!gThreadsInitialized) return;
    gMainFiber->Release(); gMainFiber = nullptr;
    gMainThread->Release(); gMainThread = nullptr;
    gThreadsInitialized = false;
}
void radThreadCreateMutex(IRadThreadMutex** mutex, radMemoryAllocator) { *mutex = new MacMutex(); }
void radThreadCreateSemaphore(IRadThreadSemaphore** semaphore, unsigned int count, radMemoryAllocator) { *semaphore = new MacSemaphore(count); }
void radThreadCreateThread(IRadThread** thread, RADTHREADENTRY entry, void* userData, IRadThread::Priority priority, unsigned int, radMemoryAllocator) { *thread = new MacThread(entry, userData, priority); }
IRadThread* radThreadGetActiveThread(void) { return MacThread::ActiveThread() != nullptr ? MacThread::ActiveThread() : gMainThread; }
void radThreadSleep(unsigned int milliseconds) { std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds)); }
void radThreadCreateLocalStorage(IRadThreadLocalStorage** storage, radMemoryAllocator) { *storage = new MacThreadLocalStorage(); }
void radThreadCreateFiber(IRadThreadFiber** fiber, RADFIBERENTRY entry, void* userData, unsigned int, radMemoryAllocator) { *fiber = new MacFiber(entry, userData); }
IRadThreadFiber* radThreadGetActiveFiber(void) { return gMainFiber; }
