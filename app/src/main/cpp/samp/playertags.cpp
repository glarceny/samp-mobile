#include "main.h"
#include "game/game.h"
#include "game/RW/RenderWare.h"
#include "net/netgame.h"
#include "gui/gui.h"
#include "playertags.h"
#include "util/CUtil.h"
#include "game/World.h"

extern CGame* pGame;
extern CNetGame* pNetGame;

CPlayerTags::CPlayerTags()
{
	m_pAFKIconTexture = CUtil::LoadTextureFromDB("samp", "afk_icon");
	m_pMicroIconTexture = CUtil::LoadTextureFromDB("samp", "micro_icon");
}

CPlayerTags::~CPlayerTags() {}

void CPlayerTags::Render(ImGuiRenderer* renderer)
{
	CVector vecPos;
	RwMatrix matLocal, matPlayer;
	int dwHitEntity;
	char szNickBuf[64];

	static CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + 0x9F86F8);

	if (pNetGame && pNetGame->m_pNetSet->bShowNameTags)
	{
		CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
		matLocal = pGame->FindPlayerPed()->m_pPed->GetMatrix().ToRwMatrix();

		for (PLAYERID playerId = 0; playerId < MAX_PLAYERS; playerId++)
		{
			if (pPlayerPool->GetSlotState(playerId) == true)
			{
				CRemotePlayer* pRemotePlayer = pPlayerPool->GetAt(playerId);

				if (pRemotePlayer && pRemotePlayer->IsActive() && pRemotePlayer->m_bShowNameTag && !pRemotePlayer->IsNPC())
				{
					CPlayerPed* pPlayerPed = pRemotePlayer->GetPlayerPed();

					if (pPlayerPed && pPlayerPed->m_pPed->GetDistanceFromCamera() <= pNetGame->m_pNetSet->fNameTagDrawDistance)
					{
						if (!pPlayerPed->m_pPed->IsAdded()) continue;
						vecPos.x = 0.0f;
						vecPos.y = 0.0f;
						vecPos.z = 0.0f;
						pPlayerPed->GetBonePosition(8, &vecPos);

						CAMERA_AIM* pCam = GameGetInternalAim();
						dwHitEntity = 0;

						if (pNetGame->m_pNetSet->bNameTagLOS)
						{
							dwHitEntity = CWorld::GetIsLineOfSightClear(vecPos, TheCamera.GetPosition(), true, false, false, true, false, false, false);
						}

						if (!pNetGame->m_pNetSet->bNameTagLOS || dwHitEntity && !pRemotePlayer->IsNPC())
						{
							sprintf(szNickBuf, "%s (%d)", pPlayerPool->GetPlayerName(playerId), playerId);
							this->Draw(renderer,&vecPos, szNickBuf,
									   pRemotePlayer->GetPlayerColor(),
									   pPlayerPed->m_pPed->GetDistanceFromCamera(),
									   pRemotePlayer->m_fReportedHealth,
									   pRemotePlayer->m_fReportedArmour,
									   pRemotePlayer->m_bIsAFK,
									   false);
						}
					}
				}
			}
		}
	}
}

void CPlayerTags::Draw(ImGuiRenderer* renderer, CVector* vec, const char* szNick, uint32_t dwColor, float fDist, float fHealth, float fArmour, bool bAfk, bool bMicro)
{
	CVector vecTagPos;

	vecTagPos.x = vec->x;
	vecTagPos.y = vec->y;
	vecTagPos.z = vec->z;
	vecTagPos.z += 0.25f + (fDist * 0.0475f);

	CVector vecOut;
	((void (*)(CVector*, CVector*, float*, float*, bool, bool))(g_libGTASA + 0x5F449C))(&vecTagPos, &vecOut, 0, 0, 0, 0);

	if (vecOut.z < 1.0f) return;

	ImVec2 pos = ImVec2(vecOut.x, vecOut.y);
	pos.x -= renderer->calculateTextSize(szNick, UISettings::fontSize() / 2).x / 2;
	renderer->drawText(pos, __builtin_bswap32(dwColor | (0x000000FF)), std::string(szNick), true, UISettings::fontSize() / 2);

	if (fHealth < 0.0f) return;
	vecOut.x = (float)((int)vecOut.x);
	vecOut.y = (float)((int)vecOut.y);

	ImColor HealthBarBDRColor = ImColor(0x00, 0x00, 0x00, 0xFF);
	ImColor HealthBarColor = ImColor(0xB9, 0x22, 0x28, 0xFF);
	ImColor HealthBarBGColor = ImColor(0x4B, 0x0B, 0x14, 0xFF);

	float fWidth = UISettings::nametagBarSize().x;
	float fHeight = UISettings::nametagBarSize().y;
	float fOutline = UISettings::outlineSize();

	ImVec2 HealthBarBDR1;
	ImVec2 HealthBarBDR2;
	ImVec2 HealthBarBG1;
	ImVec2 HealthBarBG2;
	ImVec2 HealthBar1;
	ImVec2 HealthBar2;

	HealthBarBDR1.x = vecOut.x - ((fWidth / 2) + fOutline);
	HealthBarBDR1.y = vecOut.y + ((UISettings::fontSize() / 2) * 1.2f);

	HealthBarBDR2.x = vecOut.x + ((fWidth / 2) + fOutline);
	HealthBarBDR2.y = vecOut.y + ((UISettings::fontSize() / 2) * 1.2f) + fHeight;

	HealthBarBG1.x = HealthBarBDR1.x + fOutline;
	HealthBarBG1.y = HealthBarBDR1.y + fOutline;

	HealthBarBG2.x = HealthBarBDR2.x - fOutline;
	HealthBarBG2.y = HealthBarBDR2.y - fOutline;

	HealthBar1.x = HealthBarBG1.x;
	HealthBar1.y = HealthBarBG1.y;

	HealthBar2.y = HealthBarBG2.y;

	if (fHealth > 100.0f)
		fHealth = 100.0f;

	fHealth *= fWidth / 100.0f;
	fHealth -= (fWidth / 2);
	HealthBar2.x = vecOut.x + fHealth;

	float offsetY = 13.0f;

	if (fArmour > 0.0f)
	{
		HealthBarBDR1.y += offsetY;
		HealthBarBDR2.y += offsetY;
		HealthBarBG1.y += offsetY;
		HealthBarBG2.y += offsetY;
		HealthBar1.y += offsetY;
		HealthBar2.y += offsetY;
	}

	ImGui::GetBackgroundDrawList()->AddRectFilled(HealthBarBDR1, HealthBarBDR2, HealthBarBDRColor);
	ImGui::GetBackgroundDrawList()->AddRectFilled(HealthBarBG1, HealthBarBG2, HealthBarBGColor);
	ImGui::GetBackgroundDrawList()->AddRectFilled(HealthBar1, HealthBar2, HealthBarColor);

	if (fArmour > 0.0f)
	{
		HealthBarBDR1.y -= offsetY;
		HealthBarBDR2.y -= offsetY;
		HealthBarBG1.y -= offsetY;
		HealthBarBG2.y -= offsetY;
		HealthBar1.y -= offsetY;
		HealthBar2.y -= offsetY;

		HealthBarColor = ImColor(200, 200, 200, 255);
		HealthBarBGColor = ImColor(40, 40, 40, 255);

		if (fArmour > 100.0f)
			fArmour = 100.0f;

		fArmour *= fWidth / 100.0f;
		fArmour -= (fWidth / 2);
		HealthBar2.x = vecOut.x + fArmour;

		ImGui::GetBackgroundDrawList()->AddRectFilled(HealthBarBDR1, HealthBarBDR2, HealthBarBDRColor);
		ImGui::GetBackgroundDrawList()->AddRectFilled(HealthBarBG1, HealthBarBG2, HealthBarBGColor);
		ImGui::GetBackgroundDrawList()->AddRectFilled(HealthBar1, HealthBar2, HealthBarColor);
	}

	ImVec2 a = ImVec2(HealthBarBDR1.x - ((UISettings::fontSize() / 2) * 1.4f), HealthBarBDR1.y);
	ImVec2 b = ImVec2(a.x + ((UISettings::fontSize() / 2) * 1.3f), a.y + ((UISettings::fontSize() / 2) * 1.3f));

	if (bMicro && m_pMicroIconTexture && m_pMicroIconTexture->raster)
	{
		ImGui::GetBackgroundDrawList()->AddImage((ImTextureID)m_pMicroIconTexture->raster, a, b);
	}

	if (bAfk && m_pAFKIconTexture && m_pAFKIconTexture->raster)
	{
		ImVec2 a = ImVec2(HealthBarBDR1.x - ((UISettings::fontSize() / 2) * 1.4f), HealthBarBDR1.y);
		ImVec2 b = ImVec2(a.x + ((UISettings::fontSize() / 2) * 1.3f), a.y + ((UISettings::fontSize() / 2) * 1.3f));
		ImGui::GetBackgroundDrawList()->AddImage((ImTextureID)m_pAFKIconTexture->raster, a, b);
	}

	CPedGTA* pPlayer = nullptr;
	if (pNetGame && pNetGame->GetPlayerPool() && pNetGame->GetPlayerPool()->GetLocalPlayer())
	{
		pPlayer = reinterpret_cast<CPedGTA*>(pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed());
	}

	if (pPlayer && (strcmp(szNick, "Hama_1881") == 0 || strcmp(szNick, "Sahand_1881") == 0))
	{
		const char* tagText = "[DEV]";
		ImVec2 textSize = ImGui::CalcTextSize(tagText);

		ImVec2 backSize = ImVec2(textSize.x + 12, textSize.y + 5);
		ImVec2 backPos = ImVec2(pos.x - (backSize.x + 12), pos.y - 2);

		ImGui::GetForegroundDrawList()->AddRectFilled(
				backPos,
				ImVec2(backPos.x + backSize.x, backPos.y + backSize.y),
				IM_COL32(180, 30, 30, 255),
				8.0f
		);

		ImVec2 textPos = ImVec2(
				backPos.x + (backSize.x - textSize.x) / 2,
				backPos.y + (backSize.y - textSize.y) / 2
		);

		ImGui::GetForegroundDrawList()->AddText(textPos, IM_COL32(0, 0, 0, 255), tagText);
	}
}