// Prepared by Ekaterina Ilchenko in 2022
//
// Для сборки exe: cl server.cpp ws2_32.lib
#include "Server.h"

Server::Server()
{
    IsServerActive = false;
    IsServerConfigured = InitWSA();
    numOfClients = 0;
    CurrentlySpeakingClient = ALL_CLIENTS_MUTED;
}

bool Server::InitWSA()
{
    // initialize
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR)
    {
        std::cout << "Error in WSAStartup()!\n";
        return false;
    }

    // creating a socket
    soc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (soc == INVALID_SOCKET)
    {
        std::cout << "Error when trying to create a socket!\n" << WSAGetLastError();
        WSACleanup();
        return false;
    }
    return true;
}

// to remove from a vector
void Server::DisconnectClient(ForClient* client)
{
    for (int i = 0; i < con.size(); i++)
    {
        if (con[i].numOfClient == client->numOfClient &&
            con[i].getsock == client->getsock)
            con.erase(con.cbegin() + i);
    }
}

int Server::SendCodeToAllOtherClients(const int IgnoreClientID, int Code)
{
    int RecieveCode = 4;

    for (ForClient CurrentClient : con)
    {
        if (CurrentClient.numOfClient == IgnoreClientID)
        {
            continue;  // pass sender
        }

        switch (Code)
        {
            case MUTE_CODE:
                if (!SendMuteCodeToClient(CurrentClient.getsock))
                {
                    std::cout << "Can not send MuteCode to client: "
                              << CurrentClient.numOfClient << std::endl;
                    RecieveCode = SOCKET_ERROR;
                }
                break;
            case SPEAK_CODE:
                if (!SendSpeakCodeToClient(CurrentClient.getsock))
                {
                    std::cout << "Can not send SpeakCode to client: "
                              << CurrentClient.numOfClient << std::endl;
                    RecieveCode = SOCKET_ERROR;
                }
                break;
        }

        if (RecieveCode < 0 || RecieveCode == SOCKET_ERROR)
        {
            std::cout << "Connection interrupted\n";
            return SOCKET_ERROR;
        }
        // std::cout << "Sent to customer number " << CurrentClient.numOfClient << " : ";
        // std::cout << Array << std::endl;
    }
    return RecieveCode;
}

bool Server::SendMuteCodeToClient(SOCKET& ClientSocket)
{
    int MuteCode = MUTE_CODE;
    std::cout << "Send MUTE_CODE to " << ClientSocket << std::endl;
    return send(ClientSocket, (char*)&MuteCode, sizeof(int), 0) != SOCKET_ERROR;
}

bool Server::SendSpeakCodeToClient(SOCKET& ClientSocket)
{
    int SpeakCode = SPEAK_CODE;
    std::cout << "Send SPEAK_CODE to " << ClientSocket << std::endl;
    return send(ClientSocket, (char*)&SpeakCode, sizeof(int), 0) != SOCKET_ERROR;
}

bool Server::StartServer(const char* IP, unsigned short Port)
{
    sockaddr_in ser;
    ser.sin_family = AF_INET;
    ser.sin_addr.s_addr = inet_addr(IP);
    ser.sin_port = htons(Port);
    if (bind(soc, reinterpret_cast<SOCKADDR*>(&ser), sizeof(ser)) == SOCKET_ERROR)
    {
        std::cout << "Error when binding a socket to a port!\n";
        closesocket(soc);
        IsServerActive = false;
        return false;
    }
    // listening
    if (listen(soc, 1) == SOCKET_ERROR)
    {
        std::cout << "Error listening to socket!\n";
        IsServerActive = false;
        return false;
    }
    IsServerActive = true;
    new std::thread(&Server::WaitingForTheClientToConnect, this);
    return true;
}

void Server::WaitingForTheClientToConnect()
{
    std::cout << "Server waiting for the client to connect...\n";
    while (IsServerActive)
    {
        // we accept the connection
        SOCKET Sock;
        while (IsServerActive)
        {
            Sock = SOCKET_ERROR;
            while (Sock == SOCKET_ERROR)
            {
                struct sockaddr_in client;
                int len = sizeof(client);
                Sock = accept(soc, (struct sockaddr*)&client, &len);
                ForClient* data =
                    new ForClient{Sock, client, numOfClients};  // creating a client
                con.push_back(ForClient(Sock, client, numOfClients));
                numOfClients++;
                if (numOfClients == 1000)
                    numOfClients = 0;
                new std::thread(&Server::Mail, this, data);
            }
            break;
        }
    }
}

// Function for receiving and sending messages to clients
void Server::Mail(ForClient* g)
{
    ForClient* data = reinterpret_cast<ForClient*>(g);
    SOCKET getsock = data->getsock;
    struct sockaddr_in soccl = data->soccl;
    int clientNum = data->numOfClient;
    std::cout << "Client number : " << clientNum
              << " connected with IP: " << inet_ntoa(soccl.sin_addr) << std::endl;
    if (CurrentlySpeakingClient != ALL_CLIENTS_MUTED)
    {
        SendMuteCodeToClient(getsock);
    }

    // Получение данных от клиента
    ClientHandler(getsock, clientNum);

    std::cout << "Client number: " << clientNum
              << "\ndisconnected with IP: " << inet_ntoa(soccl.sin_addr) << std::endl;
    DisconnectClient(data);
    closesocket(getsock);
    delete (data);
}

void Server::ClientHandler(SOCKET& ClientSocket, const int ClientID)
{
    // Если не смогли получить сообщение от клиента MaxConnectedTry раз считаем что клиент
    // отключился
    const int MaxConnectedTry = 50;
    int ConnectionTry = 0;  // Текущая попытка подключения
    while (ConnectionTry < MaxConnectedTry)
    {
        int ArraySize;
        int* Array = RecieveClientMessage(ClientSocket, ArraySize);
        if (ArraySize < 0)
        {
            switch (ArraySize)
            {
                case DISCONNECT_CODE_1:
                    std::cout << "Client send disconnect code\n";
                    // Условие выхода из внешнего цикла
                    ConnectionTry = MaxConnectedTry + 1;
                    break;
                case RECIEVE_DATA_ERROR_CODE:
                    ConnectionTry++;
                    break;
                case MUTE_CODE:
                    std::cout << "Recive mute code from: " << ClientID << std::endl;
                    if (CurrentlySpeakingClient == ClientID)
                    {
                        std::cout << "Mute all!" << std::endl;
                        CurrentlySpeakingClient = ALL_CLIENTS_MUTED;
                    }
                    break;
                case SPEAK_CODE:
                    if (CurrentlySpeakingClient == ALL_CLIENTS_MUTED ||
                        CurrentlySpeakingClient == ClientID)
                    {
                        CurrentlySpeakingClient = ClientID;
                        std::cout << ClientSocket << std::endl;
                        if (!SendSpeakCodeToClient(ClientSocket))
                        {
                            std::cout << "Can not send SPEAK_CODE to client: " << ClientID
                                      << std::endl;
                            CurrentlySpeakingClient = ALL_CLIENTS_MUTED;
                        }
                    }
                    else
                    {
                        if (!SendMuteCodeToClient(ClientSocket))
                        {
                            std::cout << "Can not send MUTE_CODE to client: " << ClientID
                                      << std::endl;
                        }
                    }
                    break;
                default:
                    ConnectionTry++;
                    break;
            }
        }
        else
        {
            ConnectionTry = 0;
            if (SendMessageToAllOtherClients(Array, ArraySize, ClientID) == SOCKET_ERROR)
            {
                std::cout << "Client number: " << ClientID << " Can not Send Array\n";
            };
        }
        delete[] Array;
    }
    if (CurrentlySpeakingClient == ClientID)
    {
        CurrentlySpeakingClient = ALL_CLIENTS_MUTED;
    }
}

int* Server::RecieveClientMessage(SOCKET& ClientSocket, int& SizeOfIntArray)
{
    SizeOfIntArray = RECIEVE_DATA_ERROR_CODE;
    int RecieveCode = SOCKET_ERROR;
    int InputMessageSize;

    // Получаем размер принимаемого сообщения
    RecieveCode =
        recv(ClientSocket, reinterpret_cast<char*>(&InputMessageSize), sizeof(int), 0);

    std::cout << "Client: " << ClientSocket
              << "; InputMessageSize: " << InputMessageSize << std::endl;
    if (RecieveCode == SOCKET_ERROR)
    {
        std::cout << "Recive message Size Error!\n";

        SizeOfIntArray = RECIEVE_DATA_ERROR_CODE;
        return nullptr;
    }
    if (InputMessageSize == SPEAK_CODE || InputMessageSize == MUTE_CODE)
    {
        SizeOfIntArray = InputMessageSize;
        return nullptr;
    }
    if (InputMessageSize < 0)
    {
        SizeOfIntArray = RECIEVE_DATA_ERROR_CODE;
        return nullptr;
    }
    SizeOfIntArray = InputMessageSize / sizeof(int);

    int* InputMessage = new int[SizeOfIntArray];
    // Получаем сам массив
    RecieveCode = recv(ClientSocket, reinterpret_cast<char*>(&InputMessage[0]),
                       InputMessageSize, 0);
    std::cout << "Client: " << ClientSocket << "; InputMessage: " << InputMessage
              << std::endl;
    if (RecieveCode == SOCKET_ERROR)
    {
        std::cout << "Recive message Error!\n";
        SizeOfIntArray = RECIEVE_DATA_ERROR_CODE;
        return nullptr;
    }
    return InputMessage;
}

int Server::SendMessageToClient(SOCKET& ClientSocket, const int* Array,
                                const int ArraySize)
{
    int RecieveCode = SOCKET_ERROR;
    int MessageSize = ArraySize * sizeof(int);
    std::cout << "Send array to client " << ClientSocket
              << "; MessageSize: " << MessageSize;
    RecieveCode =
        send(ClientSocket, (char*)&MessageSize, sizeof(int), 0);
    std::cout << "; RecieveCode1 " << RecieveCode;
    if (RecieveCode == SOCKET_ERROR)
    {
        return SOCKET_ERROR;
    }
    std::cout << ";Array: " << Array << std::endl;
    RecieveCode = send(ClientSocket, (char*)&Array[0], MessageSize, 0);
    return RecieveCode;
}

int Server::SendMessageToAllOtherClients(const int* Array, const int ArraySize,
                                         const int IgnoreClientID)
{
    int RecieveCode = 4;

    for (ForClient CurrentClient : con)
    {
        if (CurrentClient.numOfClient == IgnoreClientID)
        {
            continue;  // pass sender
        }

        RecieveCode = SendMessageToClient(CurrentClient.getsock, Array, ArraySize);

        if (RecieveCode < 0 || RecieveCode == SOCKET_ERROR)
        {
            std::cout << "Connection interrupted\n";
            return SOCKET_ERROR;
        }
        // std::cout << "Sent to customer number " << CurrentClient.numOfClient << " : ";
        // std::cout << Array << std::endl;
    }
    return RecieveCode;
}
