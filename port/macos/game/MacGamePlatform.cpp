#include "MacGamePlatform.h"

#include <input/inputmanager.h>
#include <radcontroller.hpp>
#include <raddebug.hpp>

// The PC data files use the game's custom Pure3D chunks.  Creating the macOS
// OpenGL platform is not enough: these handlers turn the chunks into gameplay
// objects (including the articulated physics objects used by every vehicle).
#include <p3d/file.hpp>
#include <p3d/png.hpp>
#include <p3d/bmp.hpp>
#include <p3d/targa.hpp>
#include <p3d/texture.hpp>
#include <p3d/shader.hpp>
#include <p3d/camera.hpp>
#include <p3d/gameattr.hpp>
#include <p3d/light.hpp>
#include <p3d/locator.hpp>
#include <p3d/image.hpp>
#include <p3d/imagefont.hpp>
#include <p3d/sprite.hpp>
#include <p3d/anim/skeleton.hpp>
#include <p3d/anim/polyskin.hpp>
#include <p3d/anim/compositedrawable.hpp>
#include <p3d/anim/vertexanimkey.hpp>
#include <p3d/anim/multicontroller.hpp>
#include <p3d/anim/animatedobject.hpp>
#include <p3d/anim/expression.hpp>
#include <p3d/scenegraph/scenegraph.hpp>
#include <p3d/effects/particleloader.hpp>
#include <p3d/effects/opticloader.hpp>
#include <p3d/anim/sequencer.hpp>
#include <p3d/texturefont.hpp>

#include <render/RenderManager/RenderManager.h>
#include <render/Loaders/AllWrappers.h>
#include <render/Loaders/GeometryWrappedLoader.h>
#include <render/Loaders/StaticEntityLoader.h>
#include <render/Loaders/StaticPhysLoader.h>
#include <render/Loaders/TreeDSGLoader.h>
#include <render/Loaders/FenceLoader.h>
#include <render/Loaders/IntersectLoader.h>
#include <render/Loaders/AnimCollLoader.h>
#include <render/Loaders/AnimDSGLoader.h>
#include <render/Loaders/DynaPhysLoader.h>
#include <render/Loaders/InstStatPhysLoader.h>
#include <render/Loaders/InstStatEntityLoader.h>
#include <render/Loaders/WorldSphereLoader.h>
#include <render/Loaders/BillboardWrappedLoader.h>
#include <render/Loaders/instparticlesystemloader.h>
#include <render/Loaders/breakableobjectloader.h>
#include <render/Loaders/AnimDynaPhysLoader.h>
#include <render/Loaders/lensflareloader.h>
#include <loading/locatorloader.h>
#include <loading/cameradataloader.h>
#include <loading/roadloader.h>
#include <loading/pathloader.h>
#include <loading/intersectionloader.h>
#include <loading/roaddatasegmentloader.h>
#include <atc/atcloader.h>
#include <stateprop/statepropdata.hpp>
#include <constants/srrchunks.h>
#include <simcommon/simutility.hpp>

MacGamePlatform::MacGamePlatform()
    : mInitialized(false)
{
    mErrorState = NONE;
}

MacGamePlatform::~MacGamePlatform()
{
    ShutdownPlatform();
}

void MacGamePlatform::InitializePlatform()
{
    if (mInitialized)
        return;
    InitializePure3D();
    InitializeFoundationDrive();
    GetInputManager()->Init();
    mInitialized = true;
}

void MacGamePlatform::ShutdownPlatform()
{
    if (!mInitialized)
        return;
    radControllerTerminate();
    ShutdownFoundation();
    mInitialized = false;
}

void MacGamePlatform::LaunchDashboard()
{
    // macOS has no console dashboard.  The Cocoa owner performs termination.
    rReleasePrintf("macOS: requested return to dashboard\n");
}

void MacGamePlatform::ResetMachine()
{
    LaunchDashboard();
}

void MacGamePlatform::DisplaySplashScreen(SplashScreen, const char* overlayText,
    float, float, float, tColour, int)
{
    if (overlayText != NULL)
        rReleasePrintf("macOS splash: %s\n", overlayText);
}

void MacGamePlatform::DisplaySplashScreen(const char*, const char* overlayText,
    float fontScale, float textPosX, float textPosY, tColour textColour, int fadeFrames)
{
    DisplaySplashScreen(Error, overlayText, fontScale, textPosX, textPosY, textColour, fadeFrames);
}

void MacGamePlatform::OnControllerError(const char* msg)
{
    DisplaySplashScreen(Error, msg);
    mErrorState = CTL_ERROR;
    mPauseForError = true;
}

bool MacGamePlatform::OnDriveError(radFileError error, const char*, void*)
{
    if (error == Success)
    {
        mErrorState = NONE;
        mPauseForError = false;
        return true;
    }
    mErrorState = P_ERROR;
    mPauseForError = true;
    return true;
}

void MacGamePlatform::InitializeFoundationDrive()
{
    if (mpIRadDrive != NULL)
        return;
    char driveName[radFileDrivenameMax + 1] = {};
    radGetDefaultDrive(driveName);
    radDriveOpenSync(&mpIRadDrive, driveName, NormalPriority, RADMEMORY_ALLOC_DEFAULT);
    if (mpIRadDrive != NULL)
        mpIRadDrive->RegisterErrorHandler(this, NULL);
}

void MacGamePlatform::ShutdownFoundation()
{
    if (mpIRadDrive != NULL)
    {
        mpIRadDrive->Release();
        mpIRadDrive = NULL;
    }
}

void MacGamePlatform::InitializePure3D()
{
    P3DASSERT(p3d::context);

    tP3DFileHandler* p3d = new(GMA_PERSISTENT) tP3DFileHandler;
    p3d::context->GetLoadManager()->AddHandler(p3d, "p3d");
    p3d::context->GetLoadManager()->AddHandler(new(GMA_PERSISTENT) tPNGHandler, "png");
    p3d::context->GetLoadManager()->AddHandler(new(GMA_PERSISTENT) tBMPHandler, "p3d");
    p3d::context->GetLoadManager()->AddHandler(new(GMA_PERSISTENT) tPNGHandler, "p3d");
    p3d::context->GetLoadManager()->AddHandler(new(GMA_PERSISTENT) tTargaHandler, "p3d");

#define HMR_ADD_WRAPPED_LOADER(Type, Slot) \
    do { Type* loader = (Type*)GetAllWrappers()->mpLoader(AllWrappers::Slot); \
         loader->SetRegdListener(GetRenderManager(), 0); p3d->AddHandler(loader); } while (0)
    HMR_ADD_WRAPPED_LOADER(GeometryWrappedLoader, msGeometry);
    HMR_ADD_WRAPPED_LOADER(StaticEntityLoader, msStaticEntity);
    HMR_ADD_WRAPPED_LOADER(StaticPhysLoader, msStaticPhys);
    HMR_ADD_WRAPPED_LOADER(TreeDSGLoader, msTreeDSG);
    HMR_ADD_WRAPPED_LOADER(FenceLoader, msFenceEntity);
    HMR_ADD_WRAPPED_LOADER(IntersectLoader, msIntersectDSG);
    HMR_ADD_WRAPPED_LOADER(AnimCollLoader, msAnimCollEntity);
    HMR_ADD_WRAPPED_LOADER(AnimDSGLoader, msAnimEntity);
    HMR_ADD_WRAPPED_LOADER(DynaPhysLoader, msDynaPhys);
    HMR_ADD_WRAPPED_LOADER(InstStatPhysLoader, msInstStatPhys);
    HMR_ADD_WRAPPED_LOADER(InstStatEntityLoader, msInstStatEntity);
    HMR_ADD_WRAPPED_LOADER(LocatorLoader, msLocator);
    HMR_ADD_WRAPPED_LOADER(RoadLoader, msRoadSegment);
    HMR_ADD_WRAPPED_LOADER(PathLoader, msPathSegment);
    HMR_ADD_WRAPPED_LOADER(WorldSphereLoader, msWorldSphere);
    HMR_ADD_WRAPPED_LOADER(LensFlareLoader, msLensFlare);
    HMR_ADD_WRAPPED_LOADER(BillboardWrappedLoader, msBillboard);
    HMR_ADD_WRAPPED_LOADER(InstParticleSystemLoader, msInstParticleSystem);
    HMR_ADD_WRAPPED_LOADER(BreakableObjectLoader, msBreakableObject);
    HMR_ADD_WRAPPED_LOADER(AnimDynaPhysLoader, msAnimDynaPhys);
    HMR_ADD_WRAPPED_LOADER(AnimDynaPhysWrapperLoader, msAnimDynaPhysWrapper);
#undef HMR_ADD_WRAPPED_LOADER

    p3d->AddHandler(new(GMA_PERSISTENT) tTextureLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tSetLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tShaderLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tCameraLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tGameAttrLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tLightLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tLocatorLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tLightGroupLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tImageLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tTextureFontLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tImageFontLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tSpriteLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tSkeletonLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tPolySkinLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tCompositeDrawableLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tVertexAnimKeyLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tAnimationLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tFrameControllerLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tMultiControllerLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tAnimatedObjectFactoryLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tAnimatedObjectLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tParticleSystemFactoryLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tParticleSystemLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tLensFlareGroupLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) sg::Loader);
    p3d->AddHandler(new(GMA_PERSISTENT) tExpressionGroupLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tExpressionMixerLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tExpressionLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) ATCLoader);
    p3d::loadManager->AddHandler(new(GMA_PERSISTENT) tSEQFileHandler, "seq");

    sim::InstallSimLoaders();
    p3d->AddHandler(new(GMA_PERSISTENT) CameraDataLoader, SRR2::ChunkID::FOLLOWCAM);
    p3d->AddHandler(new(GMA_PERSISTENT) CameraDataLoader, SRR2::ChunkID::WALKERCAM);
    p3d->AddHandler(new(GMA_PERSISTENT) IntersectionLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) RoadDataSegmentLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) CStatePropDataLoader);
}

void MacGamePlatform::ShutdownPure3D()
{
}
