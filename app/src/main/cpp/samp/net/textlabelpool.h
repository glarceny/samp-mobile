#pragma once

#include "../gui/gui.h"

struct TEXT_LABEL
{
    std::string text;
    uint32_t dwColor;
    CVector vecPos;
    float fDistance;
    uint8_t bTestLOS;
    PLAYERID playerId;
    VEHICLEID vehicleId;

    TEXT_LABEL() : dwColor(0), fDistance(0.0f), bTestLOS(0),
                   playerId(INVALID_PLAYER_ID), vehicleId(INVALID_VEHICLE_ID) {}
};

class C3DTextLabelPool
{
public:
    C3DTextLabelPool();
    ~C3DTextLabelPool();

    bool GetSlotState(uint16_t wLabelId) const {
        return (wLabelId < MAX_TEXT_LABELS && m_bSlotUsed[wLabelId]);
    }

    void NewLabel(uint16_t wLabelId, const TEXT_LABEL* label);
    void ClearLabel(uint16_t wLabelId);

    void Render(ImGuiRenderer* renderer);

private:
    TEXT_LABEL* m_TextLabels[MAX_TEXT_LABELS];
    bool m_bSlotUsed[MAX_TEXT_LABELS];

    void Draw(ImGuiRenderer* renderer, TEXT_LABEL* label, const CVector& vecPos,
              const std::string& text, uint32_t dwColor);
};