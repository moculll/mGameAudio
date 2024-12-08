

#include "mGameAudio.h"

#define DEBUG_PRINT(fmt, ...) \
            do { \
                printf("[mGameAudio.cpp][%s.%d]" fmt "\r\n", __func__, __LINE__, ##__VA_ARGS__); \
            } while(0);

using namespace DirectX;



bool MGameAudio::init()
{
    if (!instance) {
        instance = new MGameAudio;
    }
    HRESULT hr;
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        DEBUG_PRINT("CoInitializeEx failed, result: %ld", hr);
        return false;
    }

    
    if (FAILED(hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
        DEBUG_PRINT("XAudio2Create failed, result: %ld", hr);
        return false;
    }
        

    if (FAILED(hr = pXAudio2->CreateMasteringVoice(&pMasterVoice))) {
        DEBUG_PRINT("CreateMasteringVoice failed, result: %ld", hr);
        return false;
    }

    pMasterVoice->GetVoiceDetails(&details);

    if (FAILED(hr = pMasterVoice->GetChannelMask(&dwChannelMask))) {
        DEBUG_PRINT("GetChannelMask failed, result: %ld", hr);
        return false;
    }

    
    if (FAILED(hr = XAudio2CreateReverb(&pReverbEffect, 0))) {
        DEBUG_PRINT("XAudio2CreateReverb failed, result: %ld", hr);
        return false;
    }

    effects[0] = {pReverbEffect.Get(), TRUE, 1};
    effectChain = { 1, effects };

    if (FAILED(hr = pXAudio2->CreateSubmixVoice(&pSubmixVoice, details.InputChannels, details.InputSampleRate, 0, 0, nullptr, &effectChain))) {
        DEBUG_PRINT("CreateSubmixVoice failed, result: %ld", hr);
        return false;
    }

    ReverbConvertI3DL2ToNative(&g_PRESET_PARAMS[0], &native);
    if (FAILED(hr = pSubmixVoice->SetEffectParameters(0, &native, sizeof(native)))) {
        DEBUG_PRINT("SetEffectParameters failed, result: %ld", hr);
        return false;
    }

    if (FAILED(hr = X3DAudioInitialize(dwChannelMask, X3DAUDIO_SPEED_OF_SOUND, x3DInstance))) {
        DEBUG_PRINT("X3DAudioInitialize failed, result: %ld", hr);
        return false;
    }
    DEBUG_PRINT("MGameAudio initilized.");
    return true;

}

bool MGameAudio::createNewVoice(std::string voiceName, std::string voicePath)
{
    HRESULT hr;
    auto it = voiceMap.find(voiceName);
    if (it != voiceMap.end()) {
        DEBUG_PRINT("create new voice failed, voiceName already exists.");
        return false;
    }
    XAUDIO2_SEND_DESCRIPTOR sendDescriptors[2];
    sendDescriptors[0].Flags = XAUDIO2_SEND_USEFILTER;
    sendDescriptors[0].pOutputVoice = pSubmixVoice;
    sendDescriptors[1].Flags = XAUDIO2_SEND_USEFILTER;
    sendDescriptors[1].pOutputVoice = pMasterVoice;
    const XAUDIO2_VOICE_SENDS sendList = { 2, sendDescriptors };

    std::wstring voicePathWstr(voicePath.begin(), voicePath.end());

    /* this buffer's lifetime should be the same as your application! */
    std::unique_ptr<uint8_t[]> waveData;
    
    XAUDIO2_BUFFER buffer = { 0 };
    const WAVEFORMATEX *wfx;
    const uint8_t* sampleData;
    uint32_t waveSize;
    
    if (FAILED(hr = LoadWAVAudioFromFile(voicePathWstr.c_str(), waveData, &wfx, &sampleData, &waveSize))) {
        DEBUG_PRINT("LoadWAVAudioFromFile failed, result: %ld", hr);
        return false;
    }
    else {
        DEBUG_PRINT("LoadWAVAudioFromFile: %s successful.", voicePath.c_str());
    }
    buffer.AudioBytes = waveSize;  //size of the audio buffer in bytes
    buffer.pAudioData = sampleData;  //buffer containing audio data
    buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer
    buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

    X3DAUDIO_LISTENER listener;
    X3DAUDIO_EMITTER emitter;

    IXAudio2SourceVoice* pSourceVoice;


    listener.Position.x = 0;
    listener.Position.y = 0;
    listener.Position.z = 0;

    listener.OrientFront.x =
        listener.OrientFront.y =
        listener.OrientTop.x =
        listener.OrientTop.z = 0.f;

    listener.OrientFront.z =
        listener.OrientTop.y = 1.f;

    listener.pCone = (X3DAUDIO_CONE*)&Listener_DirectionalCone;

    emitter.pCone = &emitterCone;
    emitter.pCone->InnerAngle = 0.0f;
    // Setting the inner cone angles to X3DAUDIO_2PI and
    // outer cone other than 0 causes
    // the emitter to act like a point emitter using the
    // INNER cone settings only.
    emitter.pCone->OuterAngle = 0.0f;
    // Setting the outer cone angles to zero causes
    // the emitter to act like a point emitter using the
    // OUTER cone settings only.
    emitter.pCone->InnerVolume = 0.0f;
    emitter.pCone->OuterVolume = 1.0f;
    emitter.pCone->InnerLPF = 0.0f;
    emitter.pCone->OuterLPF = 1.0f;
    emitter.pCone->InnerReverb = 0.0f;
    emitter.pCone->OuterReverb = 1.0f;
    emitter.Velocity.x = 0;
    emitter.Velocity.y = 0;
    emitter.Velocity.z = 0;
    emitter.Position.x = 0;
    emitter.Position.y = 0;
    emitter.Position.z = 0;

    emitter.OrientFront.x =
        emitter.OrientFront.y =
        emitter.OrientTop.x =
        emitter.OrientTop.z = 0.f;

    emitter.OrientFront.z =
        emitter.OrientTop.y = 1.f;

    emitter.ChannelCount = 1;
    emitter.ChannelRadius = 1.0f;

    emitter.InnerRadius = 2.0f;
    emitter.InnerRadiusAngle = X3DAUDIO_PI / 4.0f;

    emitter.pVolumeCurve = (X3DAUDIO_DISTANCE_CURVE*)&X3DAudioDefault_LinearCurve;
    emitter.pLFECurve = (X3DAUDIO_DISTANCE_CURVE*)&Emitter_LFE_Curve;
    emitter.pLPFDirectCurve = nullptr; // use default curve
    emitter.pLPFReverbCurve = nullptr; // use default curve
    emitter.pReverbCurve = (X3DAUDIO_DISTANCE_CURVE*)&Emitter_Reverb_Curve;
    emitter.CurveDistanceScaler = 14.0f;
    emitter.DopplerScaler = 1.0f;

    dspSettings.SrcChannelCount = 1;
    dspSettings.DstChannelCount = 4;
    dspSettings.pMatrixCoefficients = matrixCoefficients;
    DWORD dwCalcFlags = X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER
        | X3DAUDIO_CALCULATE_LPF_DIRECT | X3DAUDIO_CALCULATE_LPF_REVERB
        | X3DAUDIO_CALCULATE_REVERB;
    X3DAudioCalculate(x3DInstance, &listener, &emitter, dwCalcFlags,
        &dspSettings);
    if (FAILED(hr = pXAudio2->CreateSourceVoice(&pSourceVoice, wfx, 0, 2.0f, nullptr, &sendList))) {
        DEBUG_PRINT("CreateSourceVoice failed, result: %ld", hr);
        return false;
    }
    
    if (FAILED(hr = pSourceVoice->SetFrequencyRatio(dspSettings.DopplerFactor))) {
        DEBUG_PRINT("SetFrequencyRatio failed, result: %ld", hr);
        return false;
    }
    if (FAILED(hr = pSourceVoice->SetOutputMatrix(pMasterVoice, 1, details.InputChannels, matrixCoefficients))) {
        DEBUG_PRINT("SetOutputMatrix failed, result: %ld", hr);
        return false;
    }

    if (FAILED(hr = pSourceVoice->SetOutputMatrix(pSubmixVoice, 1, 1, &dspSettings.ReverbLevel))) {
        DEBUG_PRINT("SetOutputMatrix failed, result: %ld", hr);
        return false;
    }

    XAUDIO2_FILTER_PARAMETERS FilterParametersDirect = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * dspSettings.LPFDirectCoefficient), 1.0f }; // see XAudio2CutoffFrequencyToRadians() in XAudio2.h for more information on the formula used here
    if (FAILED(hr = pSourceVoice->SetOutputFilterParameters(pMasterVoice, &FilterParametersDirect))) {
        DEBUG_PRINT("SetOutputFilterParameters failed, result: %ld", hr);
        return false;
    }
    XAUDIO2_FILTER_PARAMETERS FilterParametersReverb = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * dspSettings.LPFReverbCoefficient), 1.0f }; // see XAudio2CutoffFrequencyToRadians() in XAudio2.h for more information on the formula used here
    if (FAILED(hr = pSourceVoice->SetOutputFilterParameters(pSubmixVoice, &FilterParametersReverb))) {
        DEBUG_PRINT("SetOutputFilterParameters failed, result: %ld", hr);
        return false;
    }


    if (FAILED(hr = pSourceVoice->SubmitSourceBuffer(&buffer))) {
        DEBUG_PRINT("SubmitSourceBuffer failed, result: %ld", hr);
        return false;
    }
    if (FAILED(hr = pSourceVoice->Start(0))) {
        DEBUG_PRINT("Start failed, result: %ld", hr);
        return false;
    }

    voiceMap.insert(std::make_pair(voiceName, std::make_pair(std::move(waveData), pSourceVoice)));

    return true;
}






int main()
{
    MGameAudio::init();
    MGameAudio::createNewVoice("test", "test.wav");
    Sleep(2 * 1000);
    MGameAudio::createNewVoice("test1", "test1.wav");
    while(1);
}
