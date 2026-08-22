#include "CPhysical.h"
#include "game/Timer.h"
#include "patch.h"

CPhysical::CPhysical() : CEntityGTA()
{
    m_pCollisionList.m_pNode = nullptr;

    CPlaceable::AllocateStaticMatrix();
    m_matrix->SetUnity();

    ResetMoveSpeed();
    ResetTurnSpeed();
    m_vecFrictionMoveSpeed.Set(0.0f, 0.0f, 0.0f);
    m_vecFrictionTurnSpeed.Set(0.0f, 0.0f, 0.0f);
    m_vecForce.Set(0.0f, 0.0f, 0.0f);
    m_vecTorque.Set(0.0f, 0.0f, 0.0f);

    m_fMass = 1.0f;
    m_fTurnMass = 1.0f;
    m_fVelocityFrequency = 1.0f;
    m_fAirResistance = 0.1f;
    m_pMovingList = 0;
    m_nFakePhysics = 0;
    m_nNumEntitiesCollided = 0;
    std::fill(std::begin(m_apCollidedEntities), std::end(m_apCollidedEntities), nullptr);

    m_nPieceType = 0;

    m_fDamageIntensity = 0.0f;
    m_pDamageEntity = nullptr;

    m_vecLastCollisionImpactVelocity.Set(0.0f, 0.0f, 0.0f);
    m_vecLastCollisionPosn.Set(0.0f, 0.0f, 0.0f);

    m_bUsesCollision = true;

    m_vecCentreOfMass.Set(0.0f, 0.0f, 0.0f);

    m_fMovingSpeed = 0.0f;
    m_pAttachedTo = nullptr;
    m_pEntityIgnoredCollision = nullptr;

    m_qAttachedEntityRotation = CQuaternion(0.0f, 0.0f, 0.0f, 0.0f);

    m_fDynamicLighting = 0.0f;
    m_pShadowData = 0;
    m_fPrevDistFromCam = 100.0f;

    m_nPhysicalFlags = 0;
    physicalFlags.bApplyGravity = true;

    m_nContactSurface = SURFACE_DEFAULT;
    m_fContactSurfaceBrightness = 1.0f;
}

CPhysical::~CPhysical() {
}

bool CPhysical::IsAdded()
{
    if(this)
    {
        if(*(uintptr*)this == g_libGTASA + 0x81E628)
            return false;

        if(*(uintptr_t*)m_pMovingList)
            return true;
    }

    return false;
}

void CPhysical::ApplyMoveForce(CVector force)
{
    if (!physicalFlags.bInfiniteMass && !physicalFlags.bDisableMoveForce) {
        if (physicalFlags.bDisableZ)
            force.z = 0.0f;
        m_vecMoveSpeed += force / m_fMass;
    }
}

void CPhysical::ApplyTurnForce(CVector force, CVector point)
{
    if (!physicalFlags.bDisableTurnForce)
    {
        CVector vecCentreOfMassMultiplied{};
        if (!physicalFlags.bInfiniteMass)
            vecCentreOfMassMultiplied = Multiply3x3(GetMatrix(), m_vecCentreOfMass);

        if (physicalFlags.bDisableMoveForce) {
            point.z = 0.0f;
            force.z = 0.0f;
        }
        CVector vecDifference = point - vecCentreOfMassMultiplied;
        m_vecTurnSpeed += CrossProduct(vecDifference, force) / m_fTurnMass;
    }
}

void CPhysical::ApplyMoveSpeed()
{
    if (physicalFlags.bDontApplySpeed || physicalFlags.bDisableMoveForce)
        ResetMoveSpeed();
    else
        GetPosition() += CTimer::GetTimeStep() * m_vecMoveSpeed;
}

CVector CPhysical::GetSpeed(CVector point)
{
    CVector vecCentreOfMassMultiplied;
    if (!physicalFlags.bInfiniteMass)
        vecCentreOfMassMultiplied = Multiply3x3(GetMatrix(), m_vecCentreOfMass);

    CVector distance = point - vecCentreOfMassMultiplied;
    CVector vecTurnSpeed = m_vecTurnSpeed + m_vecFrictionTurnSpeed;
    CVector speed = CrossProduct(vecTurnSpeed, distance);
    speed += m_vecMoveSpeed + m_vecFrictionMoveSpeed;
    return speed;
}

void CPhysical::ApplyMoveForce(float x, float y, float z)
{
    return ApplyMoveForce(CVector(x, y ,z));
}

void CPhysical::ApplyTurnSpeed()
{
    if (physicalFlags.bDontApplySpeed) {
        ResetTurnSpeed();
    }
    else
    {
        CVector vecTurnSpeedTimeStep = CTimer::GetTimeStep() * m_vecTurnSpeed;
        CVector vecCrossProduct;
        CrossProduct(&vecCrossProduct, &vecTurnSpeedTimeStep, &GetRight());
        GetRight() += vecCrossProduct;
        CrossProduct(&vecCrossProduct, &vecTurnSpeedTimeStep, &GetForward());
        GetForward() += vecCrossProduct;
        CrossProduct(&vecCrossProduct, &vecTurnSpeedTimeStep, &GetUp());
        GetUp() += vecCrossProduct;
        if (!physicalFlags.bInfiniteMass && !physicalFlags.bDisableMoveForce) {
            CVector vecNegativeCentreOfMass = m_vecCentreOfMass * -1.0f;
            CVector vecCentreOfMassMultiplied = Multiply3x3(GetMatrix(), vecNegativeCentreOfMass);
            GetPosition() += CrossProduct(vecTurnSpeedTimeStep, vecCentreOfMassMultiplied);
        }
    }
}

void CPhysical::Add() {
    CHook::CallFunction<void>(g_libGTASA + 0x4A09B0, this);
}

void CPhysical::Remove() {
    CHook::CallFunction<void>(g_libGTASA + 0x4A0B88, this);
}

CRect CPhysical::GetBoundRect()
{
    CRect rect;
    if (!this || !m_pRwObject || m_nModelIndex == 0xFFFF) {
        rect.left = 0.0f;
        rect.right = 0.0f;
        rect.top = 0.0f;
        rect.bottom = 0.0f;
        return rect;
    }
    return CHook::CallFunction<CRect>(g_libGTASA + 0x4A0940, this);
}

CVector CPhysical::GetBoundCentre()
{
    if (!this || !m_pRwObject || m_nModelIndex == 0xFFFF) {
        return CVector(0.0f, 0.0f, 0.0f);
    }
    return CHook::CallFunction<CVector>(g_libGTASA + 0x4A0978, this);
}