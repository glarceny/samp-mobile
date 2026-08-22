#include "../main.h"
#include "game.h"
#include "RW/RenderWare.h"
#include "Streaming.h"
#include "Scene.h"
#include "VisibilityPlugins.h"
#include "game/Models/ModelInfo.h"
#include "game/Plugins/RpAnimBlendPlugin/RpAnimBlend.h"
#include "CRenderTarget.h"
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
    if (iModel > 20000) {
        FLog("Snapshot: model %d too big, abort", iModel);
        return nullptr;
    }

    FLog("Snapshot: START model=%d rot=(%.2f, %.2f, %.2f) zoom=%.2f",
         iModel, vecRot->x, vecRot->y, vecRot->z, fZoom);

    // --- загрузка модели ---
    if (!CStreaming::TryLoadModel(iModel)) {
        FLog("SNAPSHOT Model not loaded");
        return nullptr;
    }

    auto pRwObject = ModelInfoCreateInstance(iModel);
    if (!pRwObject) {
        FLog("Snapshot: ModelInfoCreateInstance FAILED for model %d", iModel);
        return nullptr;
    }

    // --- создание растров ---
    RwRaster* raster = RwRasterCreate(256, 256, 32, rwRASTERFORMAT8888 | rwRASTERTYPECAMERATEXTURE);
    if (!raster) {
        DestroyAtomicOrClump(reinterpret_cast<uintptr_t>(pRwObject));
        return nullptr;
    }

    RwTexture* bufferTexture = RwTextureCreate(raster);
    if (!bufferTexture) {
        DestroyAtomicOrClump(reinterpret_cast<uintptr_t>(pRwObject));
        return nullptr;
    }
    auto pModelInfo = CModelInfo::GetModelInfo(iModel);
    float fRadius = pModelInfo->m_pColModel->GetBoundRadius();
    CVector vecCenter = pModelInfo->m_pColModel->GetBoundCenter();

    RwFrame* parent = static_cast<RwFrame*>(pRwObject->parent);
    if (!parent) {
        DestroyAtomicOrClump(reinterpret_cast<uintptr_t>(pRwObject));
        return nullptr;
    }

    float fCameraDist = (-0.1f - fRadius * 2.25f) * fZoom;
    RwV3d v = {
            -vecCenter.x,
            fCameraDist,
            50.0f - vecCenter.z
    };
    RwFrameTranslate(parent, &v, rwCOMBINEPRECONCAT);

    // --- вращение ---
    if (iModel == 18631) {
        RwV3d vec = {0.0f, 0.0f, 0.0f};
        RwFrameRotate(parent, &vec, 180.0f, rwCOMBINEPRECONCAT);
    } else {
        if (vecRot->x != 0.0f) {
            RwV3d axis = {1.0f, 0.0f, 0.0f};
            RwFrameRotate(parent, &axis, vecRot->x, rwCOMBINEPRECONCAT);
        }
        if (vecRot->y != 0.0f) {
            RwV3d axis = {0.0f, 1.0f, 0.0f};
            RwFrameRotate(parent, &axis, vecRot->y, rwCOMBINEPRECONCAT);
        }
        if (vecRot->z != 0.0f) {
            RwV3d axis = {0.0f, 0.0f, 1.0f};
            RwFrameRotate(parent, &axis, vecRot->z, rwCOMBINEPRECONCAT);
        }
    }

    m_camera->frameBuffer = raster;
    CVisibilityPlugins::SetRenderWareCamera(m_camera);

    RwCameraClear(m_camera, reinterpret_cast<RwRGBA*>(&dwColor), 3);

    RwCameraBeginUpdate(m_camera);

    RpWorldAddLight(Scene.m_pRpWorld, m_light);

    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEGOURAUD);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)0);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODENACULLMODE);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)false);

    DefinedState();
    RenderClumpOrAtomic(reinterpret_cast<uintptr_t>(pRwObject));

    RwCameraEndUpdate(m_camera);

    RpWorldRemoveLight(Scene.m_pRpWorld, m_light);

    DestroyAtomicOrClump(reinterpret_cast<uintptr_t>(pRwObject));
    return bufferTexture;
}
// 0.3.7
RwTexture* CSnapShotHelper::CreatePedSnapShot(int iModel, uint32_t dwColor, CVector* vecRot, float fZoom)
{
    FLog("Ped snapshot: %d", iModel);

    CPlayerPed* pPed = new CPlayerPed(208, 0, 0.0f, 0.0f, 0.0f, 0.0f);
    if (!pPed) return nullptr;

    float posZ = iModel == 162 ? 50.15f : 50.05f;
    float posY = fZoom * -2.25f;

    pPed->m_pPed->SetPosn(0.0f, posY, posZ);
    pPed->SetModelIndex(iModel);
    pPed->m_pPed->SetCollisionChecking(false);

    RwMatrix mat = pPed->m_pPed->GetMatrix().ToRwMatrix();
    CVector axis { 1.0f, 0.0f, 0.0f };

    if (vecRot->x != 0.0f) RwMatrixRotate(&mat, &axis, vecRot->x);
    axis.Set(0.0f, 1.0f, 0.0f);
    if (vecRot->y != 0.0f) RwMatrixRotate(&mat, &axis, vecRot->y);
    axis.Set(0.0f, 0.0f, 1.0f);
    if (vecRot->z != 0.0f) RwMatrixRotate(&mat, &axis, vecRot->z);

    pPed->m_pPed->SetMatrix((CMatrix&)mat);

    CRenderTarget::Begin(256, 256, (RwRGBA*)&dwColor, false);

        RwRaster* raster = RwRasterCreate(256, 256, 32, rwRASTERFORMAT8888 | rwRASTERTYPECAMERATEXTURE);
        RwTexture* bufferTexture = RwTextureCreate(raster);

        if (!raster) {
            delete pPed;
            return nullptr;
        }

        if(!bufferTexture)
        {
            bufferTexture = RwTextureCreate(raster);
            FLog("ped take the texture");
        }

        m_camera->frameBuffer = raster;
        CVisibilityPlugins::SetRenderWareCamera(m_camera);
        RwCameraClear(m_camera, reinterpret_cast<RwRGBA *>(&dwColor), 3);
        RwCameraBeginUpdate((RwCamera*)m_camera);

        RpWorldAddLight(Scene.m_pRpWorld, m_light);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
        RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEGOURAUD);
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)false);
        DefinedState();

        pPed->m_pPed->Add();
        RpAnimBlendClumpUpdateAnimations(pPed->m_pPed->m_pRwClump, 100.0f, 1);
        RenderEntity(pPed->m_pPed);
        pPed->m_pPed->Remove();

        RwCameraEndUpdate((RwCamera*)m_camera);
        RpWorldRemoveLight(Scene.m_pRpWorld, m_light);

        delete pPed;
        CStreaming::RemoveModelIfNoRefs(iModel);

        return bufferTexture;
}

RwTexture* CSnapShotHelper::CreateVehicleSnapShot(int iModel, uint32_t dwColor, CVector* vecRot, float fZoom, uint32_t dwColor1, uint32_t dwColor2)
{
    if (iModel == 570) iModel = 538;
    else if (iModel == 569) iModel = 537;

    CVehicle* pVehicle = new CVehicle(iModel, 0.0f, 0.0f, 50.0f, 0.0f, false, false);
    if (!pVehicle || !pVehicle->m_pVehicle) {
        FLog("Failed to create vehicle");
        return nullptr;
    }

    pVehicle->m_pVehicle->SetCollisionChecking(false);

    float radius = CModelInfo::GetModelInfo(iModel)->m_pColModel->GetBoundRadius();
    float posY = (-1.0f - (radius + radius)) * fZoom;

    if (pVehicle->GetVehicleSubtype() == VEHICLE_SUBTYPE_BOAT) {
        posY = -5.5f - radius * 2.5f;
    }

    pVehicle->m_pVehicle->SetPosn(0.0f, posY, 50.0f);

    if (dwColor1 != 0xFFFFFFFF && dwColor2 != 0xFFFFFFFF) {
        pVehicle->SetColor(dwColor1, dwColor2);
    }

    RwMatrix mat = pVehicle->m_pVehicle->GetMatrix().ToRwMatrix();
    CVector axis { 1.0f, 0.0f, 0.0f };

    if (vecRot->x != 0.0f) RwMatrixRotate(&mat, &axis, vecRot->x);
    axis.Set(0.0f, 1.0f, 0.0f);
    if (vecRot->y != 0.0f) RwMatrixRotate(&mat, &axis, vecRot->y);
    axis.Set(0.0f, 0.0f, 1.0f);
    if (vecRot->z != 0.0f) RwMatrixRotate(&mat, &axis, vecRot->z);

    pVehicle->m_pVehicle->SetMatrix((CMatrix&)mat);
    pVehicle->m_pVehicle->UpdateRW();

        CRenderTarget::Begin(256, 256, (RwRGBA*)&dwColor, false);
        RwRaster* raster = RwRasterCreate(256, 256, 32, rwRASTERFORMAT8888 | rwRASTERTYPECAMERATEXTURE);
        RwTexture* bufferTexture = RwTextureCreate(raster);

        if (!raster) {
            delete pVehicle;
            return nullptr;
        }

        if(!bufferTexture)
        {
            bufferTexture = RwTextureCreate(raster);
            FLog("veh take the texture");
        }

        m_camera->frameBuffer = raster;
        CVisibilityPlugins::SetRenderWareCamera(m_camera);
        RwCameraClear(m_camera, reinterpret_cast<RwRGBA *>(&dwColor), 3);
        RwCameraBeginUpdate((RwCamera*)m_camera);

        RpWorldAddLight(Scene.m_pRpWorld, m_light);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
        RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEGOURAUD);
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)false);
        DefinedState();

        pVehicle->m_pVehicle->Add();
        RenderEntity(pVehicle->m_pVehicle);
        pVehicle->m_pVehicle->Remove();

        RwCameraEndUpdate((RwCamera*)m_camera);
        RpWorldRemoveLight(Scene.m_pRpWorld, m_light);

        delete pVehicle;
        return bufferTexture;
}
