#pragma once

#ifdef __linux

// Инклюды специфичные для линукса

#elif defined _WIN32

#include <Windows.h>

#endif

#define DEFAULT_SERVER_IP "192.168.1.65"
#define DEFAULT_SERVER_PORT 3433
// Коды общения сервера и клиента
// каждый код должен состоять из 5 символов(не обязательно но желательно)
#define SEND_CODE_Array "ARRAY"
#define SEND_CODE_Message "_MESS"
#define SEND_CODE_Code "_CODE"

#define DISCONNECT_CODE_1 -4931
#define MUTE_CODE -4932
#define SPEAK_CODE -4933
#define ALL_CLIENTS_MUTED -1
#define RECIEVE_DATA_ERROR_CODE -1

// Structure for storing customer information
struct ForClient
{
    SOCKET getsock;
    struct sockaddr_in soccl;
    int numOfClient;
    ForClient(const SOCKET& Sock, const sockaddr_in& client, const int& numOfClients)
        : getsock(Sock), soccl(client), numOfClient(numOfClients){};
};

enum CanSpeak
{
    CanSpeakNow,
    WaitingForAnswer,
    CanNotSpeakNow
};