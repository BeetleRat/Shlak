// ClientServerClasses.cpp : Этот файл содержит функцию "main". Здесь начинается и
// заканчивается выполнение программы.
//

#include <iostream>
#include "ClientServer/Client.h"
#include "ClientServer/Server.h"

#define DEFAULT_IP "192.168.1.65"
#define DEFAULT_PORT 3443


Server MyServer;

Client MyClient;
Client MyClient1;

/// Функция считывания си строки
///
/// @param [out] string_size Ссылка на int в который буде сохранен размр считанной строки
/// @returns Строку в виде char* размером сохраненным в string_size
char* ReadString(int* string_size)
{
    *string_size = 0;
    int capacity = 1;  // Вместимость строки
    // Выделяем память под считываемую строку
    char* string = (char*)malloc(sizeof(char));
    char c = getchar();  // Считываем один символ
    while (c != '\n')    // До тех пор пока не нажали Enter
    {
        // Записываем считанный char в следующий элемент строки
        // Увеличиваем string_size на 1
        string[(*string_size)++] = c;
        // Если строка превышает текущую вместимость
        if ((*string_size) >= capacity)
        {
            // Пересоздать строку с вместимостью в два раза больше предыдущей
            capacity *= 2;
            string = (char*)realloc(string, capacity * sizeof(char));
        }
        c = getchar();  // Считываем один символ
    }
    string[(*string_size)] = '\0';  // Добавляем знак конца строки
    return string;
}

int main()
{
    std::string Answer;
    int Size;
    std::cout << "S - Start Server / R - Client Recorder\n";
    do
    {
        Answer = ReadString(&Size);
    } while (Answer[0] != 's' && Answer[0] != 'S' && Answer[0] != 'r' && Answer[0] != 'R');
    if (Answer[0] == 's' || Answer[0] == 'S')
    {        
        if (MyServer.StartServer(DEFAULT_IP, DEFAULT_PORT))
        {
            std::cout << "Server starts\n";
            while (1)
            {
            }
        }
        else
        {
            std::cout << "Cannot start server\n";
        }        
    }
    else
    {
        if (Answer[0] == 'r' || Answer[0] == 'R')
        {
            if (MyClient.ConnectToServer(DEFAULT_IP, DEFAULT_PORT, "Record1.wav"))
            {
                std::cout << "S - speak / M - mute / R - Return(end of program)\n";
                while (1)
                {
                    do
                    {
                        Answer = ReadString(&Size);
                    } while (Answer[0] != 's' && Answer[0] != 'S' && Answer[0] != 'm' &&
                             Answer[0] != 'M' && Answer[0] != 'r' && Answer[0] != 'R');
                    switch (Answer[0])
                    {
                        case 's':
                        case 'S':
                            if (MyClient.SetMute(false))
                            {
                                std::cout << "You can speak\n";
                            }
                            else
                            {
                                std::cout << "You can not speak\n";
                            }
                            break;
                        case 'm':
                        case 'M':
                            MyClient.SetMute(true);
                            break;
                        case 'r':
                        case 'R':
                            return 0;
                        default:
                            break;
                    }        
                }
    
            }
        }
    }
}
