#include "CRenderTarget.h"

#include "../main.h"
#include "game.h"
#include "Scene.h"
#include "VisibilityPlugins.h"

bool CRenderTarget::InitialiseScene()
{
    m_pLight = RpLightCreate(2);
    if (m_pLight == nullptr)
    {
        return false;
    }

    RwRGBAReal rwColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    RpLightSetColor(m_pLight, &rwColor);

    m_pCamera = RwCameraCreate();

    if (m_pCamera == nullptr)
    {
        return false;
    }

    m_pFrame = RwFrameCreate();

    if (m_pFrame == nullptr)
    {
        return false;
    }

    RwV3d v = { 0.0f, 0.0f, 50.0f };
    RwFrameTranslate(m_pFrame, &v, rwCOMBINEPRECONCAT);

    v.x = 1.0f; v.y = 0.0f; v.z = 0.0f;
    RwFrameRotate(m_pFrame, &v, 90.0f, rwCOMBINEPRECONCAT);

    m_zBuffer = RwRasterCreate(256, 256, 0, rwRASTERTYPEZBUFFER);

    if (m_zBuffer == nullptr)
    {
        return false;
    }
    m_pCamera->zBuffer = m_zBuffer;

    _rwObjectHasFrameSetFrame(m_pCamera, m_pFrame);

    RwCameraSetFarClipPlane(m_pCamera, 300.0f);

    RwCameraSetNearClipPlane(m_pCamera, 0.01f);

    RwV2d view = { 0.5f, 0.5f };
    RwCameraSetViewWindow(m_pCamera, &view);

    RwCameraSetProjection(m_pCamera, rwPERSPECTIVE);

    RpWorldAddCamera(Scene.m_pRpWorld, m_pCamera);

    return true;
}

void CRenderTarget::PreProcessCamera(RwInt32 sizeX, RwInt32 sizeY)
{
    auto pRaster = RwRasterCreate(sizeX, sizeY, 32, rwRASTERFORMAT8888 | rwRASTERTYPECAMERATEXTURE);

    m_pCamera->frameBuffer = pRaster;

    CVisibilityPlugins::SetRenderWareCamera(m_pCamera);
}

void CRenderTarget::ProcessCamera(RwRGBA* bgcolor, bool b2D)
{
    RwCameraClear(m_pCamera, bgcolor, 3);

    if (!RwCameraBeginUpdate(m_pCamera))
    {
        m_bSucessfull = false;
        return;
    }
    m_bSucessfull = true;


    RpWorldAddLight(Scene.m_pRpWorld, m_pLight);

    if (b2D)
    {
        DefinedState2d();
    }
    else
    {
        DefinedState();
    }
}

void CRenderTarget::PostProcessCamera()
{
    if (m_bSucessfull)
    {
        RwCameraEndUpdate(m_pCamera);

        RpWorldRemoveLight(Scene.m_pRpWorld, m_pLight);
    }
}

void CRenderTarget::Initialise()
{
    //m_pResultTexture = nullptr;

    m_pCamera = nullptr;
    m_pLight = nullptr;
    m_pFrame = nullptr;
    m_zBuffer = 0;

    m_bReady = false;

    if (InitialiseScene())
    {
        m_bReady = true;
    }
}
//
//void CRenderTarget::ShutDown()
//{
//	// RwBool RpLightDestroy(RpLight* light);
//	((RwBool (*)(struct RpLight*))(g_libGTASA + 0x001E3810 + 1))(m_pLight);
//
//	_rwObjectHasFrameSetFrame(m_pCamera, nullptr);
//
//	// RwBool RwFrameDestroy(RwFrame* frame);
//	((RwBool(*)(struct RwFrame*))(g_libGTASA + 0x001AEC84 + 1))(m_pFrame);
//
//	RwRasterDestroy(m_zBuffer);
//	*(RwRaster**)((uintptr_t)m_pCamera + 0x64) = nullptr;
//
//	// RwBool RwCameraDestroy(RwCamera* camera);
//
//	((RwBool(*)(struct RwCamera*))(g_libGTASA + 0x001AD9D8 + 1))(m_pCamera);
//}

void CRenderTarget::Begin(RwInt32 sizeX, RwInt32 sizeY, RwRGBA* bgColor, bool b2D)
{
    if (!m_bReady)
    {
        return;
    }

    PreProcessCamera(sizeX, sizeY);
    ProcessCamera(bgColor, b2D);
}

RwTexture* CRenderTarget::End()
{
    if (!m_bReady)
    {
        return nullptr;
    }

    PostProcessCamera();

    auto tex = RwTextureCreate(m_pCamera->frameBuffer);

    ((RwTexture*(*)(struct RwTexture*, const char*))(g_libGTASA + (VER_x32 ? 0x001DB820 + 1 : 0x274000)))(tex, "rtarget");

    //m_pResultTexture = nullptr;
    m_pCamera->frameBuffer = nullptr;

    return tex;
}

