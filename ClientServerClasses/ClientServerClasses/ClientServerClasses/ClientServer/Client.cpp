// Prepared by Ekaterina Ilchenko in 2022

// Для сборки exe: cl client.cpp ws2_32.lib
#include "Client.h"

Client::Client()
{
    Mute = true;
    IsSendingThreadEnds = true;
    IsAcceptingThreadEnds = true;
    IsClientActive = false;
    IsAudioPlayingNow = false;
    RecieveTry = 0;
    MaxRecieveTry = 50;
    CanClientSpeakNow = CanSpeak::WaitingForAnswer;
    IsClientConfigured = InitWSA();

    PlayingMutex = CreateSemaphore(NULL,  // аттрибуты безопасности по умолчанию
                                   1,  // начальное значение счетчика
                                   1,  // максимаьлное значение счетчика
                                   NULL);  // безымянный семафор
    if (PlayingMutex == NULL)
        std::cout << "Ошибка создания мьютекса " << std::endl;
}

Client::~Client()
{
    DisconnectFromServer();
}

bool Client::ConnectToServer(const char* IP, unsigned short Port)
{
    if (IsClientActive)
    {
        std::cout << "Client is not active\n";
        return false;
    }
    if (!IsClientConfigured)
    {
        std::cout << "Client is not configurated\n";
        IsClientConfigured = InitWSA();
        if (!IsClientConfigured)
        {
            std::cout << "Client STILL is not configurated\n";
            return false;
        }
    }
    struct sockaddr_in fordata;
    // binding the socket
    fordata.sin_family = AF_INET;
    fordata.sin_port = htons(Port);
    fordata.sin_addr.s_addr = inet_addr(IP);
    if (connect(m_socket, reinterpret_cast<SOCKADDR*>(&fordata), sizeof(fordata)) < 0)
    {
        std::cout << "Connection error\n";
        closesocket(m_socket);
        return false;
    }

    IsClientActive = true;

    Player.StartPlaying();
    Microphone.StartRecording();
    IsSendingThreadEnds = false;
    IsAcceptingThreadEnds = false;
    new std::thread(&Client::Accepting, this);
    new std::thread(&Client::Sending, this);

    return true;
}

bool Client::ConnectToServer(const char* IP, unsigned short Port,
                             const char* RecordingFileName)
{
    if (IsClientActive)
    {
        return false;
    }
    if (!IsClientConfigured)
    {
        IsClientConfigured = InitWSA();
        if (!IsClientConfigured)
        {
            return false;
        }
    }

    struct sockaddr_in fordata;
    // binding the socket
    fordata.sin_family = AF_INET;
    fordata.sin_port = htons(Port);
    fordata.sin_addr.s_addr = inet_addr(IP);
    if (connect(m_socket, reinterpret_cast<SOCKADDR*>(&fordata), sizeof(fordata)) < 0)
    {
        std::cout << "Connection error\n";
        closesocket(m_socket);
        return false;
    }

    IsClientActive = true;

    Player.StartPlayingAndRecordingToFile(RecordingFileName);
    Microphone.StartRecording();
    IsSendingThreadEnds = false;
    IsAcceptingThreadEnds = false;
    new std::thread(&Client::Accepting, this);
    new std::thread(&Client::Sending, this);

    return true;
}

void Client::DisconnectFromServer()
{
    if (!IsClientActive || !IsClientConfigured)
    {
        std::cout << "Client is not active or config. Disconnecting\n ";
        return;
    }
    std::cout << "Disconnecting\n";
    IsClientActive = false;
    Mute = true;
    while (IsSendingThreadEnds)
    {
    }
    Player.StopPlaying();
    Microphone.StopRecording();
    // Ждем завершения всех потоков, кроме потока приема данных от сервера
    // (он завершится с закрытием сокета)
    while (!IsSendingThreadEnds || IsClientActive || Player.IsPlayingInProgress() ||
           Microphone.IsRecordInProgress() || IsAudioPlayingNow)
    {
    }
    SendDisconnectCode();
    closesocket(m_socket);
}

bool Client::SetMute(bool IsMute)
{
    if (IsMute)
    {
        Mute = IsMute;
        // Ждем отправки последнего пакета
        while (Microphone.IsRecordInProgress())
        {
        }
        SendMuteCode();
        return true;
    }
    else
    {
        Mute = !SendSpeakCode();
        if (Mute == IsMute)
        {
            return true;
        }
    }
    return false;
}

bool Client::InitWSA()
{
    // initialize
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR)
    {
        std::cout << "Error in WSAStartup()\n";
        return false;
    }

    // creating a socket
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket == INVALID_SOCKET)
    {
        std::cout << "Error when creating a socket\n" << WSAGetLastError();
        WSACleanup();
        return false;
    }
    return true;
}

int* Client::RecieveServerMessage(SOCKET& ServerSocket, int& SizeOfIntArray)
{
    SizeOfIntArray = RECIEVE_DATA_ERROR_CODE;
    int RecieveCode = SOCKET_ERROR;
    int InputMessageSize;

    // Получаем размер принимаемого сообщения
    RecieveCode =
        recv(ServerSocket, reinterpret_cast<char*>(&InputMessageSize), sizeof(int), 0);
    std::cout << "Recieve Server Message. RecieveCodeSize: " << RecieveCode
              << "; InputMessageSize: " << InputMessageSize;

    if (RecieveCode == SOCKET_ERROR)
    {
        std::cout << " Recive message Size Error!\n";
        SizeOfIntArray = RECIEVE_DATA_ERROR_CODE;
        return nullptr;
    }
    if (InputMessageSize < 0)
    {
        std::cout << " This is a code\n";
        SizeOfIntArray = InputMessageSize;
        return nullptr;
    }

    // Нужна задержка, что бы сервер успел отправить новые данные
    Sleep(1);

    SizeOfIntArray = InputMessageSize / sizeof(int);
    int* InputMessage = new int[SizeOfIntArray];
    // Получаем сам массив
    RecieveCode = recv(ServerSocket, reinterpret_cast<char*>(&InputMessage[0]),
                       InputMessageSize, 0);
    std::cout << "; RecieveCodeMessage: " << RecieveCode
              << "; InputMessage: " << InputMessage<<std::endl;

    if (RecieveCode == SOCKET_ERROR)
    {
        std::cout << "Recive message Error!\n";
        SizeOfIntArray = RECIEVE_DATA_ERROR_CODE;
        return nullptr;
    }
    return InputMessage;
}

void Client::SendIntArray(int* Array, int Size)
{
    int MessageSize = Size * sizeof(int);
    std::cout << "Send array to server. Array size: " << MessageSize;
    send(m_socket, (char*)&MessageSize, sizeof(int), 0);
    std::cout << "Array: " << Array << std::endl;
    send(m_socket, (char*)&Array[0], MessageSize, 0);
}

void Client::SendDisconnectCode()
{
    // Ждем милисекунду, что бы у сервера было время поймать код отключения
    Sleep(1);
    std::cout << "Send disconnect code" << std::endl;
    int DisconnectCode = DISCONNECT_CODE_1;
    send(m_socket, (char*)&DisconnectCode, sizeof(int), 0);
}
bool Client::SendSpeakCode()
{
    CanClientSpeakNow = CanSpeak::WaitingForAnswer;
    std::cout << "Send speak code" << std::endl;
    int SpeakCode = SPEAK_CODE;
    if (send(m_socket, (char*)&SpeakCode, sizeof(int), 0) == SOCKET_ERROR)
    {
        std::cout << "Can not send SPEAK_CODE" << std::endl;
        return false;
    }
    while (CanClientSpeakNow == CanSpeak::WaitingForAnswer)
    {
    }
    if (CanClientSpeakNow == CanSpeak::CanSpeakNow)
    {
        CanClientSpeakNow = CanSpeak::WaitingForAnswer;
        return true;
    }
    else
    {
        CanClientSpeakNow = CanSpeak::WaitingForAnswer;
        return false;
    }
}

bool Client::SendMuteCode()
{
    // Ждем милисекунду, что бы у сервера было время поймать код отключения
    Sleep(1);
    std::cout << "Send MUTE code" << std::endl;
    int MuteCode = MUTE_CODE;
    if (send(m_socket, (char*)&MuteCode, sizeof(int), 0) == SOCKET_ERROR)
    {
        std::cout << "Can not send MUTE_CODE" << std::endl;
        return false;
    }
}

void Client::PlayRecieveAudio(int* RecieveAudioArray, int ArraySize)
{
    if (Player.IsPlayingInProgress())
    {
        WaitForSingleObject(PlayingMutex, INFINITE);

        IsAudioPlayingNow = true;
        if (RecieveAudioArray != nullptr)
        {
            Player.PlayOnePart(RecieveAudioArray, ArraySize);
            delete[] RecieveAudioArray;
        }
        IsAudioPlayingNow = false;

        if (!ReleaseSemaphore(PlayingMutex,  // дескриптор семафора
                              1,  // увеличиваем значение счетчика на единицу
                              NULL))  // игнорируем предыдущее значение счетчика
        {
            printf("Release mutex error.\n");
        }
    }
}

void Client::Sending()
{
    while (IsClientActive)
    {
        Microphone.StartRecording();
        while (Microphone.IsRecordInProgress() && !Mute)
        {
            int Size = 0;
            int* AudioArray = Microphone.RecordOnePart(Size);
            if (Size > 0 && Microphone.GetOnePartSize() == Size)
            {
                SendIntArray(AudioArray, Size);
            }
        }
        Microphone.StopRecording();
    }
    IsSendingThreadEnds = true;
}

// function for accepting messages
void Client::Accepting()
{
    while (IsClientActive)
    {
        // Получение массива от сервера
        int ArraySize;
        int* AudioArray = RecieveServerMessage(m_socket, ArraySize);
        if (ArraySize < 0)
        {
            std::cout << "Recive InputMessageSize: " << ArraySize << std::endl;
            RecieveTry++;
            switch (ArraySize)
            {
                case SPEAK_CODE:
                    CanClientSpeakNow = CanSpeak::CanSpeakNow;
                    RecieveTry = 0;
                    break;
                case MUTE_CODE:
                    CanClientSpeakNow = CanSpeak::CanNotSpeakNow;
                    RecieveTry = 0;
                    break;
            }           
        }
        else
        {
            if (ArraySize != Player.GetOnePartSize())
            {
                std::cout << "Recieve Error\n";
                RecieveTry++;
                if (RecieveTry > MaxRecieveTry)
                {
                    delete[] AudioArray;
                    //IsClientActive = false;
                    DisconnectFromServer();
                    break;
                }                
                continue;
            }
            RecieveTry = 0;
            new std::thread(&Client::PlayRecieveAudio, this, AudioArray, ArraySize);
        }
    }
    IsAcceptingThreadEnds = true;
}
