#pragma once
#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <thread>
#include "ClientServerConst.h"
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable : 4996)


class Server
{
public:
    Server();
    bool StartServer(const char* IP, unsigned short Port);

private:
    DWORD idthr;
    std::vector<ForClient> con;
    SOCKET soc;
    int numOfClients;

    bool IsServerConfigured;
    bool IsServerActive;
    int CurrentlySpeakingClient;

    bool InitWSA();
    void WaitingForTheClientToConnect();

    // Method for receiving and sending messages to clients
    void Mail(ForClient* g);

    void ClientHandler(SOCKET& ClientSocket, const int ClientID);

    int* RecieveClientMessage(SOCKET& ClientSocket, int& SizeOfIntArray);
    int SendMessageToClient(SOCKET& ClientSocket, const int* Array, const int ArraySize);
    int SendMessageToAllOtherClients(const int* Array, const int ArraySize,
                                     const int IgnoreClientID);    
    void DisconnectClient(ForClient* client);

    int SendCodeToAllOtherClients(const int IgnoreClientID, int Code);
    bool SendMuteCodeToClient(SOCKET& ClientSocket);
    bool SendSpeakCodeToClient(SOCKET& ClientSocket);
};