#pragma once
/* to use xaudio2 we have to include windows.h first */
#include <windows.h>
#include <xaudio2.h>
#include <x3daudio.h>
#include <xaudio2fx.h>
#include <xapofx.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include "wavebank/WAVFileReader.h"

#include <map>
#include <string>

class MGameAudio {
    
private:
    inline static MGameAudio* instance = nullptr;

public:
    MGameAudio(){}
    ~MGameAudio() {}
    
    static bool init();
    
    static bool createNewVoice(std::string voiceName, std::string voicePath);


private:
    inline static std::map<std::string, std::pair<std::unique_ptr<uint8_t[]>, IXAudio2SourceVoice*>> voiceMap;

    

    inline static IXAudio2* pXAudio2 = nullptr;
    inline static Microsoft::WRL::ComPtr<IUnknown> pReverbEffect;
    inline static IXAudio2MasteringVoice* pMasterVoice = nullptr;
    inline static DWORD dwChannelMask = 0;
    inline static XAUDIO2_VOICE_DETAILS details;
    inline static XAUDIO2_EFFECT_DESCRIPTOR effects[1];
    inline static XAUDIO2_EFFECT_CHAIN effectChain;
    inline static XAUDIO2FX_REVERB_PARAMETERS native;

    /* can't be local variable! */
    inline static FLOAT32 matrixCoefficients[8];

    inline static IXAudio2SubmixVoice* pSubmixVoice = nullptr;


    inline static X3DAUDIO_HANDLE x3DInstance;
    inline static X3DAUDIO_DSP_SETTINGS dspSettings;

    

    inline static X3DAUDIO_LISTENER listener;
    inline static X3DAUDIO_EMITTER emitter;
    inline static DirectX::XMFLOAT3 vListenerPos;
    inline static DirectX::XMFLOAT3 vEmitterPos;
    
    
    inline static X3DAUDIO_CONE emitterCone;



    
    inline static constexpr X3DAUDIO_CONE Listener_DirectionalCone = { X3DAUDIO_PI * 5.0f / 6.0f, X3DAUDIO_PI * 11.0f / 6.0f, 1.0f, 0.75f, 0.0f, 0.25f, 0.708f, 1.0f };
    inline static constexpr X3DAUDIO_DISTANCE_CURVE_POINT Emitter_LFE_CurvePoints[3] = { {0.0f, 1.0f}, {0.25f, 0.0f}, {1.0f, 0.0f} };
    inline static constexpr X3DAUDIO_DISTANCE_CURVE Emitter_LFE_Curve = { (X3DAUDIO_DISTANCE_CURVE_POINT*)&Emitter_LFE_CurvePoints[0], 3 };
    inline static constexpr X3DAUDIO_DISTANCE_CURVE_POINT Emitter_Reverb_CurvePoints[3] = { {0.0f, 0.5f}, {0.75f, 1.0f}, {1.0f, 0.0f} };
    inline static constexpr X3DAUDIO_DISTANCE_CURVE Emitter_Reverb_Curve = { (X3DAUDIO_DISTANCE_CURVE_POINT*)&Emitter_Reverb_CurvePoints[0], 3 };
    inline static constexpr XAUDIO2FX_REVERB_I3DL2_PARAMETERS g_PRESET_PARAMS[30] =
    {
        XAUDIO2FX_I3DL2_PRESET_FOREST,
        XAUDIO2FX_I3DL2_PRESET_DEFAULT,
        XAUDIO2FX_I3DL2_PRESET_GENERIC,
        XAUDIO2FX_I3DL2_PRESET_PADDEDCELL,
        XAUDIO2FX_I3DL2_PRESET_ROOM,
        XAUDIO2FX_I3DL2_PRESET_BATHROOM,
        XAUDIO2FX_I3DL2_PRESET_LIVINGROOM,
        XAUDIO2FX_I3DL2_PRESET_STONEROOM,
        XAUDIO2FX_I3DL2_PRESET_AUDITORIUM,
        XAUDIO2FX_I3DL2_PRESET_CONCERTHALL,
        XAUDIO2FX_I3DL2_PRESET_CAVE,
        XAUDIO2FX_I3DL2_PRESET_ARENA,
        XAUDIO2FX_I3DL2_PRESET_HANGAR,
        XAUDIO2FX_I3DL2_PRESET_CARPETEDHALLWAY,
        XAUDIO2FX_I3DL2_PRESET_HALLWAY,
        XAUDIO2FX_I3DL2_PRESET_STONECORRIDOR,
        XAUDIO2FX_I3DL2_PRESET_ALLEY,
        XAUDIO2FX_I3DL2_PRESET_CITY,
        XAUDIO2FX_I3DL2_PRESET_MOUNTAINS,
        XAUDIO2FX_I3DL2_PRESET_QUARRY,
        XAUDIO2FX_I3DL2_PRESET_PLAIN,
        XAUDIO2FX_I3DL2_PRESET_PARKINGLOT,
        XAUDIO2FX_I3DL2_PRESET_SEWERPIPE,
        XAUDIO2FX_I3DL2_PRESET_UNDERWATER,
        XAUDIO2FX_I3DL2_PRESET_SMALLROOM,
        XAUDIO2FX_I3DL2_PRESET_MEDIUMROOM,
        XAUDIO2FX_I3DL2_PRESET_LARGEROOM,
        XAUDIO2FX_I3DL2_PRESET_MEDIUMHALL,
        XAUDIO2FX_I3DL2_PRESET_LARGEHALL,
        XAUDIO2FX_I3DL2_PRESET_PLATE,
    };
};

