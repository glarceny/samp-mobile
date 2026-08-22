#pragma once

class UI;

class CDebugInfo
{
public:
    CDebugInfo();
    ~CDebugInfo();

    void Render(UI* pUI);
    void Shutdown(UI* pUI);

private:
    void UpdateFPS();

    class Label* m_pLabel = nullptr;
    float m_fCurrentFPS;
    float m_fLastUpdateTime;
};