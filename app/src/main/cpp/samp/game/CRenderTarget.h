#pragma once

#include <vector>
#include <stdint.h>
#include "game/RW/RenderWare.h"

class CRenderTarget
{
//	static inline struct RwCamera* m_pOldCamera;
    static inline struct RwCamera* m_pCamera;
    static inline struct RpLight* m_pLight;
    static inline struct RwFrame* m_pFrame;
    static inline struct RwRaster* m_zBuffer;

    static inline bool m_bReady;

    static inline bool m_bSucessfull;
public:
    static bool InitialiseScene();

    static void PreProcessCamera(RwInt32 sizeX, RwInt32 sizeY);
    static void ProcessCamera(RwRGBA* bgcolor, bool b2D);
    static void PostProcessCamera();
    static void Begin(RwInt32 sizeX, RwInt32 sizeY, RwRGBA* bgColor, bool b2D);
    static struct RwTexture* End();

    static void Initialise();
};