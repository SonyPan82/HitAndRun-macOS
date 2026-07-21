// The Radical SDK's global `ref` smart pointer predates std::ref.  Load the
// standard declaration first, then isolate the legacy spelling for the old
// game translation units.
#include <functional>
#include <cctype>
#include <strings.h>
#define ref radref

static inline char* hmr_strupr(char* text)
{
    for (char* cursor = text; *cursor != '\0'; ++cursor)
    {
        *cursor = static_cast<char>(std::toupper(static_cast<unsigned char>(*cursor)));
    }
    return text;
}
#define strupr hmr_strupr
#define stricmp strcasecmp
