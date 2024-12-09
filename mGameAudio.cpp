

#include "mGameAudio.h"
#include <thread>
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
    nSampleRate = details.InputSampleRate;
    nChannels = details.InputChannels;
    dwChannelMask = dwChannelMask;
    
    if (FAILED(hr = XAudio2CreateReverb(&pReverbEffect, 0))) {
        DEBUG_PRINT("XAudio2CreateReverb failed, result: %ld", hr);
        return false;
    }

    XAUDIO2_EFFECT_DESCRIPTOR effects[] = { { pReverbEffect.Get(), TRUE, 1 } };
    XAUDIO2_EFFECT_CHAIN effectChain = { 1, effects };

    if (FAILED(hr = pXAudio2->CreateSubmixVoice(&pSubmixVoice, 1, nSampleRate, 0, 0, nullptr, &effectChain))) {
        DEBUG_PRINT("CreateSubmixVoice failed, result: %ld", hr);
        return false;
    }
    XAUDIO2FX_REVERB_PARAMETERS native;
    ReverbConvertI3DL2ToNative(&g_PRESET_PARAMS[1], &native);

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

    XAUDIO2_BUFFER buffer = {};
    buffer.AudioBytes = waveSize;
    buffer.pAudioData = sampleData;
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.LoopCount = 1;/* XAUDIO2_LOOP_INFINITE; */

    X3DAUDIO_LISTENER listener;
    X3DAUDIO_EMITTER emitter;

    IXAudio2SourceVoice* pSourceVoice;


    listener.Position.x = 
        listener.Position.y = 
        listener.Position.z = .0f;

    listener.OrientFront.x =
        listener.OrientFront.y =
        listener.OrientTop.x =
        listener.OrientTop.z = 0.f;

    listener.OrientFront.z =
        listener.OrientTop.y = 1.f;

    listener.pCone = (X3DAUDIO_CONE*)&Listener_DirectionalCone;

    emitter.pCone = &emitterCone;
    emitter.pCone->InnerAngle = 0.0f;

    emitter.pCone->OuterAngle = 0.0f;

    emitter.pCone->InnerVolume = 0.0f;
    emitter.pCone->OuterVolume = 1.0f;
    emitter.pCone->InnerLPF = 0.0f;
    emitter.pCone->OuterLPF = 1.0f;
    emitter.pCone->InnerReverb = 0.0f;
    emitter.pCone->OuterReverb = 1.0f;
    

    emitter.Position.x = 
    emitter.Position.y = 
    emitter.Position.z = .0f;

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
    emitter.pLPFDirectCurve = nullptr;
    emitter.pLPFReverbCurve = nullptr;
    emitter.pReverbCurve = (X3DAUDIO_DISTANCE_CURVE*)&Emitter_Reverb_Curve;
    emitter.CurveDistanceScaler = 14.0f;
    emitter.DopplerScaler = 1.0f;

    dspSettings.SrcChannelCount = 1;
    dspSettings.DstChannelCount = nChannels;
    dspSettings.pMatrixCoefficients = matrixCoefficients;


    if (FAILED(hr = pXAudio2->CreateSourceVoice(&pSourceVoice, wfx, 0, 2.0f, nullptr, &sendList))) {
        DEBUG_PRINT("CreateSourceVoice failed, result: %ld", hr);
        return false;
    }
    
    buffer.pAudioData = sampleData;
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.AudioBytes = waveSize;
    buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

    pSourceVoice->SubmitSourceBuffer(&buffer);

    PlayUnit newUnit(std::move(waveData), std::move(pSourceVoice), std::move(buffer), std::move(listener), std::move(emitter));
    voiceMap.insert(std::make_pair(voiceName, std::move(newUnit)));

    return true;
}


bool MGameAudio::playAudio(std::string voiceName, bool interrupt)
{

    auto it = voiceMap.find(voiceName);
    if (it == voiceMap.end()) {
        DEBUG_PRINT("playAudio failed, voiceName doesn't exists.");
        return false;
    }
    HRESULT hr;
    if (interrupt) {
        if (FAILED(hr = it->second.pSourceVoice->Stop(0))) {
            DEBUG_PRINT("Stop Failed, result: %ld", hr);
            return false;
        }
    }
    if (FAILED(hr = it->second.pSourceVoice->FlushSourceBuffers())) {
        DEBUG_PRINT("FlushSourceBuffers Failed, result: %ld", hr);
        return false;
    }
    if (FAILED(hr = it->second.pSourceVoice->SubmitSourceBuffer(&it->second.buffer))) {
        DEBUG_PRINT("SubmitSourceBuffer Failed, result: %ld", hr);
        return false;
    }

    if (FAILED(hr = it->second.pSourceVoice->Start(0))) {
        DEBUG_PRINT("Start failed, result: %ld", hr);
        return false;
    }
    return true;

}
bool MGameAudio::updateEmitterPosition(std::string voiceName, XMFLOAT3& position)
{
    auto it = voiceMap.find(voiceName);
    if (it == voiceMap.end()) {
        DEBUG_PRINT("updateEmitterPosition failed, voiceName doesn't exist.");
        return false;
    }

    HRESULT hr;

    it->second.listener.Position.x = 0.0f;
    it->second.listener.Position.y = 0.0f;
    it->second.listener.Position.z = 0.0f;
    it->second.listener.Velocity.x = it->second.listener.Velocity.y = it->second.listener.Velocity.z = 1.0f;

    it->second.listener.OrientFront.x = 0.0f;
    it->second.listener.OrientFront.y = 0.0f;
    it->second.listener.OrientFront.z = 1.0f;

    it->second.listener.OrientTop.x = 0.0f;
    it->second.listener.OrientTop.y = 1.0f;
    it->second.listener.OrientTop.z = 0.0f;

    it->second.listener.pCone = (X3DAUDIO_CONE*)&Listener_DirectionalCone;

    it->second.emitter.Position = position;

    it->second.emitter.OrientFront.x = 0.0f;
    it->second.emitter.OrientFront.y = 0.0f;
    it->second.emitter.OrientFront.z = 1.0f;
    it->second.emitter.OrientTop.x = 0.0f;
    it->second.emitter.OrientTop.y = 1.0f;
    it->second.emitter.OrientTop.z = 0.0f;

    it->second.emitter.pCone = &emitterCone;
    it->second.emitter.pCone->InnerAngle = 0.0f;
    it->second.emitter.pCone->OuterAngle = 0.0f;
    it->second.emitter.pCone->InnerVolume = 0.0f;
    it->second.emitter.pCone->OuterVolume = 1.0f;
    it->second.emitter.pCone->InnerLPF = 0.0f;
    it->second.emitter.pCone->OuterLPF = 1.0f;
    it->second.emitter.pCone->InnerReverb = 0.0f;
    it->second.emitter.pCone->OuterReverb = 1.0f;

    it->second.emitter.Velocity.x = 0.0f;
    it->second.emitter.Velocity.y = 0.0f;
    it->second.emitter.Velocity.z = 0.0f;

    it->second.emitter.ChannelCount = 1;
    it->second.emitter.ChannelRadius = 1.0f;

    it->second.emitter.InnerRadius = 2.0f;
    it->second.emitter.InnerRadiusAngle = X3DAUDIO_PI / 4.0f;

    it->second.emitter.pVolumeCurve = (X3DAUDIO_DISTANCE_CURVE*)&X3DAudioDefault_LinearCurve;
    it->second.emitter.pLFECurve = (X3DAUDIO_DISTANCE_CURVE*)&Emitter_LFE_Curve;
    it->second.emitter.pLPFDirectCurve = nullptr;
    it->second.emitter.pLPFReverbCurve = nullptr;
    it->second.emitter.pReverbCurve = (X3DAUDIO_DISTANCE_CURVE*)&Emitter_Reverb_Curve;
    it->second.emitter.CurveDistanceScaler = 14.0f;
    it->second.emitter.DopplerScaler = 1.0f;

    DWORD dwCalcFlags = X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER
        | X3DAUDIO_CALCULATE_LPF_DIRECT | X3DAUDIO_CALCULATE_LPF_REVERB
        | X3DAUDIO_CALCULATE_REVERB;

    dwCalcFlags |= X3DAUDIO_CALCULATE_REDIRECT_TO_LFE;

    X3DAudioCalculate(x3DInstance, &it->second.listener, &it->second.emitter, dwCalcFlags, &dspSettings);

    if (FAILED(hr = it->second.pSourceVoice->SetFrequencyRatio(dspSettings.DopplerFactor))) {
        DEBUG_PRINT("SetFrequencyRatio failed, result: %ld", hr);
        return false;
    }

    if (FAILED(hr = it->second.pSourceVoice->SetOutputMatrix(pMasterVoice, 1, details.InputChannels, matrixCoefficients))) {
        DEBUG_PRINT("SetOutputMatrix failed, result: %ld", hr);
        return false;
    }

    if (FAILED(hr = it->second.pSourceVoice->SetOutputMatrix(pSubmixVoice, 1, 1, &dspSettings.ReverbLevel))) {
        DEBUG_PRINT("SetOutputMatrix failed, result: %ld, %lf", hr, dspSettings.ReverbLevel);
        return false;
    }

    XAUDIO2_FILTER_PARAMETERS FilterParametersDirect = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * dspSettings.LPFDirectCoefficient), 1.0f };
    if (FAILED(hr = it->second.pSourceVoice->SetOutputFilterParameters(pMasterVoice, &FilterParametersDirect))) {
        DEBUG_PRINT("SetOutputFilterParameters failed, result: %ld", hr);
        return false;
    }

    XAUDIO2_FILTER_PARAMETERS FilterParametersReverb = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * dspSettings.LPFReverbCoefficient), 1.0f };
    if (FAILED(hr = it->second.pSourceVoice->SetOutputFilterParameters(pSubmixVoice, &FilterParametersReverb))) {
        DEBUG_PRINT("SetOutputFilterParameters failed, result: %ld", hr);
        return false;
    }

    return true;
}

void MGameAudio::printData()
{

    DEBUG_PRINT("%.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f", dspSettings.LPFDirectCoefficient, dspSettings.LPFReverbCoefficient \
    , dspSettings.ReverbLevel, dspSettings.DopplerFactor, dspSettings.EmitterToListenerAngle, dspSettings.EmitterToListenerDistance, dspSettings.EmitterVelocityComponent, dspSettings.ListenerVelocityComponent);

}


bool MGameAudio::updateListenerPosition(std::string voiceName, XMFLOAT3 &position)
{
    auto it = voiceMap.find(voiceName);
    if (it == voiceMap.end()) {
        DEBUG_PRINT("updateListenerPosition failed, voiceName doesn't exists.");
        return false;
    }


}


void MGameAudio::runTestCase()
{
    MGameAudio::init();
    MGameAudio::createNewVoice("test", "test.wav");
    MGameAudio::createNewVoice("test1", "test1.wav");



    XMFLOAT3 emitterPos = { 0, 0, 0 };

    std::thread updateThread = std::thread([&emitterPos]() {
        while (1) {

            MGameAudio::updateEmitterPosition("test1", emitterPos);
            MGameAudio::updateEmitterPosition("test", emitterPos);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        });
    std::thread playThread = std::thread([&emitterPos]() {
        float angle = 0.0f;
        float speed = 0.1f;
        const float RADIUS = 10.0f;
        const float CENTER_X = 0.0f;
        float CENTER_Y = 0.0f;
        const float CENTER_Z = 0.0f;

        while (true) {

            emitterPos.x = CENTER_X + RADIUS * cos(angle);
            emitterPos.y = CENTER_Y;
            emitterPos.z = CENTER_Z + RADIUS * sin(angle);
            angle += speed;
            if (angle >= 2 * X3DAUDIO_PI) {
                angle -= 2 * X3DAUDIO_PI;
            }
            DEBUG_PRINT("x: %.2f, y: %.2f, z: %.2f", emitterPos.x, emitterPos.y, emitterPos.z);

            std::this_thread::sleep_for(std::chrono::milliseconds(16));

        }


        });


    updateThread.detach();
    playThread.detach();
    while (1) {
        MGameAudio::printData();
        MGameAudio::playAudio("test", true);
        MGameAudio::playAudio("test1", true);
        Sleep(1000);
    }


}

//int main()
//{
//    MGameAudio::runTestCase();
//}
