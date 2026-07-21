#include <pddi/pddi.hpp>

int main()
{
    pddiDevice* device = nullptr;
    if (pddiCreate(PDDI_VERSION_MAJOR, PDDI_VERSION_MINOR, &device) != PDDI_OK || device == nullptr)
    {
        return 1;
    }

    pddiLibInfo info{};
    device->GetLibraryInfo(&info);
    return info.libID == PDDI_LIBID_OPENGL ? 0 : 2;
}
