#include "../main.h"
#include "../game/game.h"
#include "netgame.h"
#include "game/World.h"
#include "textlabelpool.h"

extern CGame* pGame;
extern CNetGame* pNetGame;

C3DTextLabelPool::C3DTextLabelPool()
{
    for (int i = 0; i < MAX_TEXT_LABELS; i++) {
        m_TextLabels[i] = nullptr;
        m_bSlotUsed[i] = false;
    }
}

C3DTextLabelPool::~C3DTextLabelPool()
{
    for (int i = 0; i < MAX_TEXT_LABELS; i++) {
        if (m_bSlotUsed[i]) {
            this->ClearLabel(i);
        }
    }
}

void C3DTextLabelPool::NewLabel(uint16_t wLabelId, const TEXT_LABEL* pLabel)
{
    if (wLabelId >= MAX_TEXT_LABELS || !pLabel) {
        return;
    }

    if (m_TextLabels[wLabelId]) {
        delete m_TextLabels[wLabelId];
        m_TextLabels[wLabelId] = nullptr;
        m_bSlotUsed[wLabelId] = false;
    }

    TEXT_LABEL* pTextLabel = new TEXT_LABEL();

    if (!pLabel->text.empty()) {
        pTextLabel->text = Encoding::cp2utf(pLabel->text);
    }

    pTextLabel->dwColor = pLabel->dwColor;
    pTextLabel->vecPos.x = pLabel->vecPos.x;
    pTextLabel->vecPos.y = pLabel->vecPos.y;
    pTextLabel->vecPos.z = pLabel->vecPos.z;
    pTextLabel->fDistance = pLabel->fDistance;
    pTextLabel->bTestLOS = pLabel->bTestLOS;
    pTextLabel->playerId = pLabel->playerId;
    pTextLabel->vehicleId = pLabel->vehicleId;

    m_TextLabels[wLabelId] = pTextLabel;
    m_bSlotUsed[wLabelId] = true;
}

void C3DTextLabelPool::ClearLabel(uint16_t wLabelId)
{
    if (wLabelId >= MAX_TEXT_LABELS) {
        return;
    }

    m_bSlotUsed[wLabelId] = false;

    if (m_TextLabels[wLabelId]) {
        delete m_TextLabels[wLabelId];
        m_TextLabels[wLabelId] = nullptr;
    }
}

void C3DTextLabelPool::Render(ImGuiRenderer* renderer)
{
    if (!renderer || !pGame || !pNetGame) return;

    CPlayerPed* pPlayerPed = pGame->FindPlayerPed();
    if (!pPlayerPed || !pPlayerPed->m_pPed) return;

    CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
    if (!pPlayerPool) return;

    CLocalPlayer* pLocalPlayer = pPlayerPool->GetLocalPlayer();
    if (!pLocalPlayer) return;

    CPlayerPed* pLocalPed = pLocalPlayer->GetPlayerPed();
    if (!pLocalPed || !pLocalPed->m_pPed) return;

    static CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));

    for (int i = 0; i < MAX_TEXT_LABELS; i++)
    {
        if (!m_bSlotUsed[i]) continue;

        TEXT_LABEL* pTextLabel = m_TextLabels[i];
        if (!pTextLabel) {
            m_bSlotUsed[i] = false;
            continue;
        }

        CVector vecTextPos = pTextLabel->vecPos;

        if (pTextLabel->playerId != INVALID_PLAYER_ID) {
            if (pTextLabel->playerId == pPlayerPool->GetLocalPlayerID()) continue;

            if (pPlayerPool->GetSlotState(pTextLabel->playerId)) {
                CRemotePlayer* pPlayer = pPlayerPool->GetAt(pTextLabel->playerId);
                if (pPlayer) {
                    float fDist = pPlayer->GetDistanceFromLocalPlayer();
                    if (fDist < pTextLabel->fDistance && fDist >= 0.0f) {
                        CPlayerPed* pRemotePed = pPlayer->GetPlayerPed();
                        if (pRemotePed && pRemotePed->m_pPed && pRemotePed->m_pPed->IsAdded()) {
                            CVector matBone;
                            pRemotePed->GetBonePosition(8, &matBone);
                            vecTextPos.x = matBone.x + pTextLabel->vecPos.x;
                            vecTextPos.y = matBone.y + pTextLabel->vecPos.y;
                            vecTextPos.z = matBone.z + 0.23f + pTextLabel->vecPos.z;
                            this->Draw(renderer, pTextLabel, vecTextPos, pTextLabel->text, pTextLabel->dwColor);
                        }
                    }
                }
            }
        }

        if (pTextLabel->vehicleId != INVALID_VEHICLE_ID) {
            CVehiclePool* pVehiclePool = pNetGame->GetVehiclePool();
            if (pVehiclePool && pVehiclePool->GetSlotState(pTextLabel->vehicleId)) {
                CVehicle* pVehicle = pVehiclePool->GetAt(pTextLabel->vehicleId);
                if (pVehicle && pVehicle->m_pVehicle && pVehicle->m_pVehicle->IsAdded()) {
                    float fDist = pVehicle->m_pVehicle->GetDistanceFromLocalPlayerPed();
                    if (fDist < pTextLabel->fDistance && fDist >= 0.0f) {
                        RwMatrix matVehicle = pVehicle->m_pVehicle->GetMatrix().ToRwMatrix();
                        vecTextPos.x = matVehicle.pos.x + pTextLabel->vecPos.x;
                        vecTextPos.y = matVehicle.pos.y + pTextLabel->vecPos.y;
                        vecTextPos.z = matVehicle.pos.z + pTextLabel->vecPos.z;
                        this->Draw(renderer, pTextLabel, vecTextPos, pTextLabel->text, pTextLabel->dwColor);
                    }
                }
            }
        }
        else if (pTextLabel->playerId == INVALID_PLAYER_ID && pTextLabel->vehicleId == INVALID_VEHICLE_ID) {
            float fDist = pLocalPed->m_pPed->GetDistanceFromPoint(pTextLabel->vecPos.x, pTextLabel->vecPos.y, pTextLabel->vecPos.z);
            if (fDist <= pTextLabel->fDistance && fDist >= 0.0f) {
                this->Draw(renderer, pTextLabel, vecTextPos, pTextLabel->text, pTextLabel->dwColor);
            }
        }
    }
}

void C3DTextLabelPool::Draw(ImGuiRenderer* renderer, TEXT_LABEL* label, const CVector& vecPos,
                            const std::string& text, uint32_t dwColor)
{
    if (!renderer || !label || !pNetGame) return;

    CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
    if (!pPlayerPool) return;

    CLocalPlayer* pLocalPlayer = pPlayerPool->GetLocalPlayer();
    if (!pLocalPlayer) return;

    CPlayerPed* pLocalPed = pLocalPlayer->GetPlayerPed();
    if (!pLocalPed || !pLocalPed->m_pPed) return;

    static CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));

    int hitEntity = 0;
    if (label->bTestLOS) {
        CVector camPos = TheCamera.GetPosition();
        hitEntity = CWorld::GetIsLineOfSightClear(vecPos, camPos, true, false, false, true, false, false, false);
    }

    if (!label->bTestLOS || hitEntity) {
        float fDist = pLocalPed->m_pPed->GetDistanceFromPoint(vecPos.x, vecPos.y, vecPos.z);
        if (fDist <= label->fDistance && fDist >= 0.0f) {
            CVector vecOut;
            typedef void (*CalcScreenCoors_t)(CVector*, CVector*, float*, float*, bool, bool);
            CalcScreenCoors_t CalcScreenCoors = (CalcScreenCoors_t)(g_libGTASA + (VER_x32 ? 0x005C57E8 + 1 : 0x6E9DF8));

            if (CalcScreenCoors) {
                CalcScreenCoors((CVector*)&vecPos, &vecOut, 0, 0, 0, 0);

                if (vecOut.z < 1.0f) return;

                std::stringstream ss_data(text);
                std::string s_row;
                float fontSize = UISettings::fontSize() / 2;
                if (fontSize <= 0.0f) fontSize = 10.0f;

                while (std::getline(ss_data, s_row, '\n')) {
                    if (s_row.empty()) continue;

                    ImVec2 sz = renderer->calculateTextSize(s_row, fontSize);
                    renderer->drawText(ImVec2(vecOut.x - (sz.x / 2), vecOut.y),
                                       __builtin_bswap32(dwColor | 0x000000FF),
                                       s_row, true, fontSize);
                    vecOut.y += fontSize;
                }
            }
        }
    }
}