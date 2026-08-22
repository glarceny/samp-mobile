#pragma once

#include "Placeable.h"
#include "Link.h"
#include "game/Enums/eEntityStatus.h"
#include "game/Enums/eEntityType.h"
#include "game/Enums/eSurfaceType.h"
#include "game/Enums/eAreaCodes.h"
#include "game/Reference.h"
#include "Rect.h"
#include "game/Models/BaseModelInfo.h"
#include "game/Enums/eModelID.h"

class CPedGTA;

struct CEntityGTA : public CPlaceable{
public:
    union {
        struct RwObject* m_pRwObject;
        struct RpClump*  m_pRwClump;
        struct RpAtomic* m_pRwAtomic;
    };

    union {
        struct {
            bool m_bUsesCollision : 1;
            bool m_bCollisionProcessed : 1;
            bool m_bIsStatic : 1;
            bool m_bHasContacted : 1;
            bool m_bIsStuck : 1;
            bool m_bIsInSafePosition : 1;
            bool m_bWasPostponed : 1;
            bool m_bIsVisible : 1;

            bool m_bIsBIGBuilding : 1;
            bool m_bRenderDamaged : 1;
            bool m_bStreamingDontDelete : 1;
            bool m_bRemoveFromWorld : 1;
            bool m_bHasHitWall : 1;
            bool m_bImBeingRendered : 1;
            bool m_bDrawLast : 1;
            bool m_bDistanceFade : 1;

            bool m_bDontCastShadowsOn : 1;
            bool m_bOffscreen : 1;
            bool m_bIsStaticWaitingForCollision : 1;
            bool m_bDontStream : 1;
            bool m_bUnderwater : 1;
            bool m_bHasPreRenderEffects : 1;
            bool m_bIsTempBuilding : 1;
            bool m_bDontUpdateHierarchy : 1;

            bool m_bHasRoadsignText : 1;
            bool m_bDisplayedSuperLowLOD : 1;
            bool m_bIsProcObject : 1;
            bool m_bBackfaceCulled : 1;
            bool m_bLightObject : 1;
            bool m_bUnimportantStream : 1;
            bool m_bTunnel : 1;
            bool m_bTunnelTransition : 1;
        };
        uint32_t m_nFlags;
    };
    uint32_t flags2;
    union {
        struct {
            uint16_t m_nRandomSeedUpperByte : 8;
            uint16_t m_nRandomSeedSecondByte : 8;
        };
        uint16_t m_nRandomSeed;
    };

    uint16_t            m_nModelIndex;
    CReference          *m_pReferences;
    CLink<CEntityGTA*>     *m_pStreamingLink;
    uint16_t            m_nScanCode;
    uint8_t             m_nIplIndex;
    eAreaCodes          m_nAreaCode;
    union {
        int32_t       m_nLodIndex;
        CEntityGTA* m_pLod;
    };
    uint8_t         m_nNumLodChildren;
    int8_t          m_nNumLodChildrenRendered;
    eEntityType     m_nType : 3;
    eEntityStatus   m_nStatus : 5;
    uint8_t         pad_0;

public:
    [[nodiscard]] bool IsPhysical() const { return m_nType > ENTITY_TYPE_BUILDING && m_nType < ENTITY_TYPE_DUMMY; }
    [[nodiscard]] bool IsNothing()  const { return m_nType == ENTITY_TYPE_NOTHING; }
    [[nodiscard]] bool IsVehicle()  const { return m_nType == ENTITY_TYPE_VEHICLE; }
    [[nodiscard]] bool IsPed()      const { return m_nType == ENTITY_TYPE_PED; }
    [[nodiscard]] bool IsObject()   const { return m_nType == ENTITY_TYPE_OBJECT; }
    [[nodiscard]] bool IsBuilding() const { return m_nType == ENTITY_TYPE_BUILDING; }
    [[nodiscard]] bool IsDummy()    const { return m_nType == ENTITY_TYPE_DUMMY; }

    CEntityGTA();
    ~CEntityGTA() override;

    virtual void Add();
    virtual void Add(const CRect* rect);
    virtual void Remove();
    virtual void SetIsStatic(bool isStatic);
    virtual void SetModelIndex(uint32_t index);
    virtual void SetModelIndexNoCreate(uint32_t index);

    virtual CRect GetBoundRect();
    virtual CVector GetBoundCentre();
    virtual float GetBoundRadius();

    auto AsPed() { return reinterpret_cast<class CPedGTA*>(this); }

    [[nodiscard]] auto GetType() const noexcept { return (eEntityType)m_nType; }
    void SetType(eEntityType type) { m_nType = type; }

    [[nodiscard]] auto GetStatus() const noexcept { return m_nStatus; }
    void SetStatus(eEntityStatus status) { m_nStatus = status; }

    void SetInterior(int interiorId, bool needRefresh = false);

    float GetDistanceFromCamera();
    float GetDistanceFromLocalPlayerPed() const;
    float GetDistanceFromPoint(float X, float Y, float Z) const;

    void SetCollisionChecking(bool bCheck);

    void UpdateRpHAnim();

    void UpdateRwFrame();
    void UpdateRW();

    RwMatrix* GetModellingMatrix();

    void DeleteRwObject();

    auto GetModelId() const { return (eModelID)m_nModelIndex; }
    CBaseModelInfo* GetModelInfo() const;

    void RegisterReference(CEntityGTA** entity);

    template<typename T>
    static void RegisterReference(T*& ref) requires std::is_base_of_v<CEntityGTA, T> {
        ref->RegisterReference(reinterpret_cast<CEntityGTA**>(&ref));
    }

    void PruneReferences();
    void ResolveReferences();

    template<typename T>
    static void CleanUpOldReference(T*& ref) requires std::is_base_of_v<CEntityGTA, T> {
        ref->CleanUpOldReference(reinterpret_cast<CEntityGTA**>(&ref));
    }

    template<typename T>
    static void ClearReference(T*& ref) requires std::is_base_of_v<CEntityGTA, T> {
        if (ref) {
            ref->CleanUpOldReference(reinterpret_cast<CEntityGTA**>(&ref));
            ref = nullptr;
        }
    }

    template<typename T, typename Y>
    requires std::is_base_of_v<CEntityGTA, T> && std::is_base_of_v<CEntityGTA, Y>
    static void ChangeEntityReference(T*& inOutRef, Y* entity) {
        ClearReference(inOutRef);
        if (entity) {
            inOutRef = entity;
            inOutRef->RegisterReference(reinterpret_cast<CEntityGTA**>(&inOutRef));
        }
    }

    void CleanUpOldReference(CEntityGTA** entity);
};
static_assert(sizeof(CEntityGTA) == (VER_x32 ? 0x3C : 0x60));