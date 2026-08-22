#include "../main.h"
#include "game.h"
#include "RW/RenderWare.h"
#include "Streaming.h"
#include "Scene.h"
#include "VisibilityPlugins.h"
#include "game/Models/ModelInfo.h"
#include "game/Plugins/RpAnimBlendPlugin/RpAnimBlend.h"
#include <GLES2/gl2.h>

extern CGame* pGame;

CSnapShotHelper::CSnapShotHelper()
{
    m_camera = 0;
    m_light = 0;
    m_frame = 0;
    m_zBuffer = 0;
    m_raster = 0;

    SetUpScene();
}

void CSnapShotHelper::SetUpScene()
{
    // RpLightCreate
    m_light = RpLightCreate(2);
    if (m_light == 0) return;
    float rwColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    // RpLightSetColor
    RpLightSetColor(m_light, reinterpret_cast<const RwRGBAReal *>(rwColor));

    m_zBuffer = RwRasterCreate(256, 256, 0, rwRASTERTYPEZBUFFER);
    m_camera = RwCameraCreate();
    m_frame = RwFrameCreate();
    CVector v = { 0.0f, 0.0f, 50.0f };
    RwFrameTranslate(m_frame, &v, rwCOMBINEPRECONCAT);
    CVector v1 = { 1.0f, 0.0f, 0.0f };
    RwFrameRotate(m_frame, &v1, 90.0f, rwCOMBINEPRECONCAT);

    if (!m_camera) return;
    if (!m_frame) return;

    m_camera->zBuffer = m_zBuffer;

    _rwObjectHasFrameSetFrame(m_camera, m_frame);
    RwCameraSetFarClipPlane(m_camera, 300.0f);
    RwCameraSetNearClipPlane(m_camera, 0.01f);
    RwV2d view = { 0.5f, 0.5f };
    RwCameraSetViewWindow(m_camera, &view);
    RwCameraSetProjection(m_camera, rwPERSPECTIVE);
    RpWorldAddCamera(Scene.m_pRpWorld, m_camera);
}

// 0.3.7
RwTexture* CSnapShotHelper::CreateObjectSnapShot(int iModel, uint32_t dwColor, CVector* vecRot, float fZoom)
{
    if(iModel > 20000 || iModel <= 0) return nullptr;

    Log("CreateObjectSnapShot: %d, %f, %f, %f", iModel, vecRot->x, vecRot->y, vecRot->z);

    // Check if model exists and is valid
    if (!CModelInfo::GetModelInfo(iModel)) {
        Log("Model %d not found", iModel);
        return nullptr;
    }

    CStreaming::TryLoadModel(iModel);

    // Wait for model to load
    int timeout = 50;
    while (!CStreaming::IsModelLoaded(iModel) && timeout > 0) {
        CStreaming::LoadAllRequestedModels(false);
        usleep(10000);
        timeout--;
    }

    if (!CStreaming::IsModelLoaded(iModel)) {
        Log("Model %d failed to load", iModel);
        CStreaming::RemoveModelIfNoRefs(iModel);
        return nullptr;
    }

    auto pRwObject = ModelInfoCreateInstance(iModel);
    if (pRwObject == nullptr) {
        Log("pRwObject no rw object");
        CStreaming::RemoveModelIfNoRefs(iModel);
        return nullptr;
    }

    RwRaster* raster = RwRasterCreate(256, 256, 32, rwRASTERFORMAT8888 | rwRASTERTYPECAMERATEXTURE);
    if (!raster) {
        Log("Failed to create raster");
        DestroyAtomicOrClump(reinterpret_cast<uintptr_t>(pRwObject));
        CStreaming::RemoveModelIfNoRefs(iModel);
        return nullptr;
    }

    // RwTextureCreate
    RwTexture* bufferTexture = RwTextureCreate(raster);
    if (!bufferTexture) {
        Log("Failed to create texture");
        RwRasterDestroy(raster);
        DestroyAtomicOrClump(reinterpret_cast<uintptr_t>(pRwObject));
        CStreaming::RemoveModelIfNoRefs(iModel);
        return nullptr;
    }

    CVector vec;
    vec.x = 0.0f;
    vec.y = 0.0f;
    vec.z = 0.0f;

    // Safely get collision model
    CColModel* pColModel = CModelInfo::GetModelInfo(iModel)->m_pColModel;
    float fRadius = 1.0f;
    CVector vecCenter = {0.0f, 0.0f, 0.0f};

    if (pColModel) {
        fRadius = pColModel->GetBoundRadius();
        vecCenter = pColModel->GetBoundCenter();
    }

    // Clamp radius to avoid extreme values
    if (fRadius <= 0.1f) fRadius = 1.0f;
    if (fRadius > 100.0f) fRadius = 10.0f;

    RwFrame* parent = static_cast<RwFrame *>(pRwObject->parent);
    if(!parent) {
        Log("No parent frame");
        if (bufferTexture) RwTextureDestroy(bufferTexture);
        DestroyAtomicOrClump(reinterpret_cast<uintptr_t>(pRwObject));
        CStreaming::RemoveModelIfNoRefs(iModel);
        return nullptr;
    }

    float fCamDistance = (-0.1f - fRadius * 2.25f) * fZoom;
    // Clamp camera distance
    if (fCamDistance > -0.5f) fCamDistance = -2.0f;
    if (fCamDistance < -50.0f) fCamDistance = -50.0f;

    if (parent)
    {
        RwV3d v = {
                -vecCenter.x + vecRot->x,
                fCamDistance + vecRot->y,
                50.0f - vecCenter.z - vecRot->z
        };
        RwFrameTranslate(parent, &v, rwCOMBINEPRECONCAT);

        // Safely apply rotations
        if (vecRot->x != 0.0f && vecRot->x >= -360.0f && vecRot->x <= 360.0f) {
            RwFrameRotate(parent, &vec, vecRot->x, rwCOMBINEPRECONCAT);
        }
        if (vecRot->y != 0.0f && vecRot->y >= -360.0f && vecRot->y <= 360.0f) {
            RwFrameRotate(parent, &vec, vecRot->y, rwCOMBINEPRECONCAT);
        }
        if (vecRot->z != 0.0f && vecRot->z >= -360.0f && vecRot->z <= 360.0f) {
            RwFrameRotate(parent, &vec, vecRot->z, rwCOMBINEPRECONCAT);
        }
    }

    m_camera->frameBuffer = raster;
    CVisibilityPlugins::SetRenderWareCamera(m_camera);

    // Clear with safety
    RwRGBA clearColor;
    clearColor.red = (dwColor >> 24) & 0xFF;
    clearColor.green = (dwColor >> 16) & 0xFF;
    clearColor.blue = (dwColor >> 8) & 0xFF;
    clearColor.alpha = dwColor & 0xFF;

    RwCameraClear(m_camera, &clearColor, rwCAMERACLEARZ | rwCAMERACLEARIMAGE);

    if (!RwCameraBeginUpdate(m_camera)) {
        Log("Failed to begin camera update");
        if (bufferTexture) RwTextureDestroy(bufferTexture);
        DestroyAtomicOrClump(reinterpret_cast<uintptr_t>(pRwObject));
        CStreaming::RemoveModelIfNoRefs(iModel);
        return nullptr;
    }

    RpWorldAddLight(Scene.m_pRpWorld, m_light);

    // Safely set render states
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEGOURAUD);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)0);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODENACULLMODE);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);

    // DefinedState
    DefinedState();

    // Safely render
    if (pRwObject) {
        RenderClumpOrAtomic((uintptr_t)pRwObject);
    }

    RwCameraEndUpdate(m_camera);
    RpWorldRemoveLight(Scene.m_pRpWorld, m_light);
    DestroyAtomicOrClump(reinterpret_cast<uintptr_t>(pRwObject));
    CStreaming::RemoveModelIfNoRefs(iModel);

    return bufferTexture;
}

// 0.3.7
RwTexture* CSnapShotHelper::CreatePedSnapShot(int iModel, uint32_t dwColor, CVector* vecRot, float fZoom)
{
    if (iModel <= 0 || iModel > 20000) return nullptr;

    Log("Ped snapshot: %d", iModel);

    // Validate and load model
    if (!CModelInfo::GetModelInfo(iModel)) {
        Log("Ped model %d not found", iModel);
        return nullptr;
    }

    CStreaming::TryLoadModel(iModel);

    int timeout = 50;
    while (!CStreaming::IsModelLoaded(iModel) && timeout > 0) {
        CStreaming::LoadAllRequestedModels(false);
        usleep(10000);
        timeout--;
    }

    if (!CStreaming::IsModelLoaded(iModel)) {
        Log("Ped model %d failed to load", iModel);
        CStreaming::RemoveModelIfNoRefs(iModel);
        return nullptr;
    }

    RwRaster* raster = RwRasterCreate(256, 256, 32, rwRASTERFORMAT8888 | rwRASTERTYPECAMERATEXTURE);
    if (!raster) {
        Log("Failed to create raster");
        CStreaming::RemoveModelIfNoRefs(iModel);
        return nullptr;
    }

    // RwTextureCreate
    RwTexture* bufferTexture = RwTextureCreate(raster);
    if (!bufferTexture) {
        Log("Failed to create texture");
        RwRasterDestroy(raster);
        CStreaming::RemoveModelIfNoRefs(iModel);
        return nullptr;
    }

    CPlayerPed* pPed = new CPlayerPed(208, 0, 0.0f, 0.0f, 0.0f, 0.0f);
    if (!pPed) {
        Log("Failed to create ped");
        RwTextureDestroy(bufferTexture);
        CStreaming::RemoveModelIfNoRefs(iModel);
        return nullptr;
    }

    float posZ = iModel == 162 ? 50.15f : 50.05f;
    float fZoomClamped = fZoom;
    if (fZoomClamped < 0.1f) fZoomClamped = 0.1f;
    if (fZoomClamped > 10.0f) fZoomClamped = 10.0f;

    float posY = fZoomClamped * -2.25f;
    if (posY > -0.5f) posY = -1.0f;
    if (posY < -20.0f) posY = -20.0f;

    pPed->m_pPed->SetPosn(0.0f, posY, posZ);
    pPed->SetModelIndex(iModel);
    pPed->m_pPed->SetCollisionChecking(false);

    RwMatrix mat = pPed->m_pPed->GetMatrix().ToRwMatrix();

    CVector axis { 1.0f, 0.0f, 0.0f };
    if (vecRot->x != 0.0f && vecRot->x >= -360.0f && vecRot->x <= 360.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->x);
    }
    axis.Set( 0.0f, 1.0f, 0.0f );
    if (vecRot->y != 0.0f && vecRot->y >= -360.0f && vecRot->y <= 360.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->y);
    }
    axis.Set( 0.0f, 0.0f, 1.0f );
    if (vecRot->z != 0.0f && vecRot->z >= -360.0f && vecRot->z <= 360.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->z);
    }

    pPed->m_pPed->SetMatrix((CMatrix&)mat);

    m_camera->frameBuffer = raster;
    CVisibilityPlugins::SetRenderWareCamera(m_camera);

    RwRGBA clearColor;
    clearColor.red = (dwColor >> 24) & 0xFF;
    clearColor.green = (dwColor >> 16) & 0xFF;
    clearColor.blue = (dwColor >> 8) & 0xFF;
    clearColor.alpha = dwColor & 0xFF;

    RwCameraClear(m_camera, &clearColor, rwCAMERACLEARZ | rwCAMERACLEARIMAGE);

    if (!RwCameraBeginUpdate(m_camera)) {
        Log("Failed to begin camera update for ped");
        if (bufferTexture) RwTextureDestroy(bufferTexture);
        delete pPed;
        CStreaming::RemoveModelIfNoRefs(iModel);
        return nullptr;
    }

    RpWorldAddLight(Scene.m_pRpWorld, m_light);

    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEGOURAUD);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);

    DefinedState();

    pPed->m_pPed->Add();

    if (pPed->m_pPed->m_pRwClump) {
        RpAnimBlendClumpUpdateAnimations(pPed->m_pPed->m_pRwClump, 100.0f, 1);
        RenderEntity(pPed->m_pPed);
    }

    RwCameraEndUpdate(m_camera);
    RpWorldRemoveLight(Scene.m_pRpWorld, m_light);
    pPed->m_pPed->Remove();
    delete pPed;
    CStreaming::RemoveModelIfNoRefs(iModel);

    return bufferTexture;
}

RwTexture* CSnapShotHelper::CreateVehicleSnapShot(int iModel, uint32_t dwColor, CVector* vecRot, float fZoom, uint32_t dwColor1, uint32_t dwColor2)
{
    if (iModel <= 0 || iModel > 20000) return nullptr;

    RwRaster* raster = RwRasterCreate(256, 256, 32, rwRASTERFORMAT8888 | rwRASTERTYPECAMERATEXTURE);
    if (!raster) return nullptr;

    // RwTextureCreate
    RwTexture* bufferTexture = RwTextureCreate(raster);
    if (!bufferTexture) {
        RwRasterDestroy(raster);
        return nullptr;
    }

    if (iModel == 570) {
        iModel = 538;
    }
    else if (iModel == 569) {
        iModel = 537;
    }

    // Validate model exists
    if (!CModelInfo::GetModelInfo(iModel)) {
        Log("Vehicle model %d not found", iModel);
        RwTextureDestroy(bufferTexture);
        return nullptr;
    }

    CStreaming::TryLoadModel(iModel);

    int timeout = 50;
    while (!CStreaming::IsModelLoaded(iModel) && timeout > 0) {
        CStreaming::LoadAllRequestedModels(false);
        usleep(10000);
        timeout--;
    }

    if (!CStreaming::IsModelLoaded(iModel)) {
        Log("Vehicle model %d failed to load", iModel);
        RwTextureDestroy(bufferTexture);
        CStreaming::RemoveModelIfNoRefs(iModel);
        return nullptr;
    }

    CVehicle* pVehicle = new CVehicle(iModel, 0.0f, 0.0f, 50.0f, 0.0f, false, false);
    if (!pVehicle || !pVehicle->m_pVehicle) {
        Log("Failed to create vehicle");
        RwTextureDestroy(bufferTexture);
        CStreaming::RemoveModelIfNoRefs(iModel);
        return nullptr;
    }

    pVehicle->m_pVehicle->SetCollisionChecking(false);

    CColModel* pColModel = CModelInfo::GetModelInfo(iModel)->m_pColModel;
    float radius = 1.0f;
    if (pColModel) {
        radius = pColModel->GetBoundRadius();
    }
    if (radius <= 0.1f) radius = 1.0f;

    float posY = (-1.0f - (radius + radius)) * fZoom;
    if (posY > -0.5f) posY = -1.0f;
    if (posY < -30.0f) posY = -30.0f;

    if (pVehicle->GetVehicleSubtype() == VEHICLE_SUBTYPE_BOAT) {
        posY = -5.5f - radius * 2.5f;
    }
    pVehicle->m_pVehicle->SetPosn(0.0f, posY, 50.0f);

    if (dwColor1 != 0xFFFFFFFF && dwColor2 != 0xFFFFFFFF) {
        pVehicle->SetColor(dwColor1, dwColor2);
    }

    RwMatrix mat = pVehicle->m_pVehicle->GetMatrix().ToRwMatrix();

    CVector axis { 1.0f, 0.0f, 0.0f };
    if (vecRot->x != 0.0f && vecRot->x >= -360.0f && vecRot->x <= 360.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->x);
    }
    axis.Set( 0.0f, 1.0f, 0.0f );
    if (vecRot->y != 0.0f && vecRot->y >= -360.0f && vecRot->y <= 360.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->y);
    }
    axis.Set( 0.0f, 0.0f, 1.0f );
    if (vecRot->z != 0.0f && vecRot->z >= -360.0f && vecRot->z <= 360.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->z);
    }

    pVehicle->m_pVehicle->SetMatrix((CMatrix&)mat);
    pVehicle->m_pVehicle->UpdateRW();

    m_camera->frameBuffer = raster;
    CVisibilityPlugins::SetRenderWareCamera(m_camera);

    RwRGBA clearColor;
    clearColor.red = (dwColor >> 24) & 0xFF;
    clearColor.green = (dwColor >> 16) & 0xFF;
    clearColor.blue = (dwColor >> 8) & 0xFF;
    clearColor.alpha = dwColor & 0xFF;

    RwCameraClear(m_camera, &clearColor, rwCAMERACLEARZ | rwCAMERACLEARIMAGE);

    if (!RwCameraBeginUpdate(m_camera)) {
        Log("Failed to begin camera update for vehicle");
        RwTextureDestroy(bufferTexture);
        delete pVehicle;
        CStreaming::RemoveModelIfNoRefs(iModel);
        return nullptr;
    }

    RpWorldAddLight(Scene.m_pRpWorld, m_light);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEGOURAUD);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);

    DefinedState();
    pVehicle->m_pVehicle->Add();
    RenderEntity(pVehicle->m_pVehicle);
    RwCameraEndUpdate(m_camera);
    RpWorldRemoveLight(Scene.m_pRpWorld, m_light);
    pVehicle->m_pVehicle->Remove();
    delete pVehicle;
    CStreaming::RemoveModelIfNoRefs(iModel);

    return bufferTexture;
}