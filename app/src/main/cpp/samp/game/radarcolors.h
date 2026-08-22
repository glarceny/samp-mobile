#pragma once

void GameResetRadarColors();
void SetRadarColor(int nIndex, uint32_t dwColor);
bool IsRadarColorCodeInRange(uint32_t code);
uint32_t TranslateColorCodeToRGBA(uint iCode);