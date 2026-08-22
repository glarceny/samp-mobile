#include "CDebugInfo.h"
#include "gui/gui.h"
#include "gui/widgets/label.h"
#include "settings.h"
#include "game/game.h"

extern CSettings* pSettings;
extern CGame* pGame;

CDebugInfo::CDebugInfo()
{
    m_fLastUpdateTime = 0;
    m_fCurrentFPS = 120.0f;
    m_pLabel = nullptr;
}

CDebugInfo::~CDebugInfo()
{
}

void CDebugInfo::Render(UI* pUI)
{
    if (!pSettings) return;
    if (!pSettings->Get().iFPSCounter) return;

    UpdateFPS();

    char szStr[30];
    snprintf(szStr, sizeof(szStr), "FPS: %.0f", m_fCurrentFPS);

    ImVec2 pos = ImVec2(pUI->ScaleX(40.0f), pUI->ScaleY(540.0f));

    if (!m_pLabel)
    {
        m_pLabel = new Label(szStr, ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
        pUI->addChild(m_pLabel);
    }

    m_pLabel->setText(szStr);
    m_pLabel->setPosition(pos);
}

void CDebugInfo::UpdateFPS()
{
    static auto lastTick = CTimer::m_snTimeInMillisecondsNonClipped;

    if (CTimer::m_snTimeInMillisecondsNonClipped - lastTick > 500)
    {
        lastTick = CTimer::m_snTimeInMillisecondsNonClipped;
        m_fCurrentFPS = std::clamp(CTimer::game_FPS, 10.0f, 120.0f);
    }
}

void CDebugInfo::Shutdown(UI* pUI)
{
    if (m_pLabel)
    {
        pUI->removeChild(m_pLabel);
        delete m_pLabel;
        m_pLabel = nullptr;
    }
}