#include <cstdarg>
#include <cstdio>

bool g_AllowDebugOutput = true;

void rReleasePrintf(const char* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    std::vfprintf(stderr, format, arguments);
    va_end(arguments);
}

bool rDebugAssertFail_Implementation(const char* condition, const char* filename, unsigned int line)
{
    std::fprintf(stderr, "ASSERT FAILED: %s (%s:%u)\n", condition, filename, line);
    // Match the non-Windows Radical implementation: callers break after this
    // return, preserving a loud failure rather than masking invalid state.
    return true;
}

void rDebugWarningFail_Implementation(const char* condition, const char* filename, unsigned int line)
{
    std::fprintf(stderr, "WARNING: %s (%s:%u)\n", condition, filename, line);
}
