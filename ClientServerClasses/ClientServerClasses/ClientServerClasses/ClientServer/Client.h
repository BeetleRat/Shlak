#pragma once
#include "ClientServerConst.h"
#include <iostream>
#include <thread>

#include "AudioRecording/AudioRecorder.h"

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable : 4996)

class Client
{
public:
    Client();
    ~Client();

    bool ConnectToServer(const char* IP, unsigned short Port);
    bool ConnectToServer(const char* IP, unsigned short Port, const char * RecordingFileName);
    void DisconnectFromServer();

    bool SetMute(bool IsMute);

private:
    SOCKET m_socket;
    AudioRecorder Microphone;
    AudioRecorder Player;
    HANDLE PlayingMutex;
    int RecieveTry;
    int MaxRecieveTry;

    bool IsClientConfigured;
    bool IsClientActive;
    bool Mute;
    CanSpeak CanClientSpeakNow;  
    bool IsSendingThreadEnds;
    bool IsAcceptingThreadEnds;
    bool IsAudioPlayingNow;

    bool InitWSA();

    int* RecieveServerMessage(SOCKET& ServerSocket, int& SizeOfIntArray);
    void SendIntArray(int* Array, int Size);
    void SendDisconnectCode();
    bool SendSpeakCode();
    bool SendMuteCode();

    void PlayRecieveAudio(int* RecieveAudioArray, int ArraySize);
    void Sending();
    // function for accepting messages
    void Accepting();
};