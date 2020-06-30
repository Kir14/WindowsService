#pragma once
#pragma comment(lib,"ws2_32.lib")
#include <WinSock2.h>
#include <iostream>
#include <WS2tcpip.h>
#include <strsafe.h>
#include <deque>
#include "pugixml.hpp"



void Connection();		//Задание порта и адресса сервера
void GetDataFromXml(std::string&, int&);	//Извлечение данных из xml
void ListenConnection();					//Прослушивание подключений к серверу
void ConnectionClient(SOCKET);				//Ф-ция для потока прослушивания клиента
void SendMessageToClient(char*);			//Рассылка сообщений клиентам
void StopServer();
void Pipes();
DWORD WINAPI InstanceThread(LPVOID);
VOID GetAnswerToRequest(LPTSTR, LPTSTR, LPDWORD);

SOCKET Connect;
std::deque<SOCKET> Connections;
std::deque<std::string>names;
SOCKET Listen;
CRITICAL_SECTION CsConn;
CRITICAL_SECTION CsNames;
int ClientMax;

HANDLE g_ServiceStopEvent = INVALID_HANDLE_VALUE;




int Server()
{
	FILE* LogFile;
	fopen_s(&LogFile, "G:\\project\\logs\\Server.log", "a");
	fprintf(LogFile, "Start\n");
	fclose(LogFile);



	InitializeCriticalSection(&CsConn);
	InitializeCriticalSection(&CsNames);

	CreateThread(
		NULL,              // no security attribute
		NULL,                 // default stack size
		(LPTHREAD_START_ROUTINE)Pipes,    // функция обработки сообщений
		NULL,    // параметр потока
		NULL,                 // not suspended
		NULL);      // возврат id потока
	
	Connection();

	fopen_s(&LogFile, "G:\\project\\logs\\Server.log", "a");
	fprintf(LogFile, "end\n");
	fclose(LogFile);

	return 0;
}


void Connection()
{

	/*
	if (FAILED (WSAStartup (MAKEWORD( 2, 2 ), &data) ) )
	{
	  // Error...
	  error = WSAGetLastError();
	  //...
	}
	*/

	WSADATA data;
	WORD version = MAKEWORD(2, 2);
	if (WSAStartup(version, &data))
	{
		return;
	}
	std::string server_ip;
	int port;
	GetDataFromXml(server_ip, port);

	sockaddr_in SAddr;
	ZeroMemory(&SAddr, sizeof(SAddr));
	// тип адреса (TCP/IP)
	SAddr.sin_family = AF_INET;
	//адрес сервера. Т.к. TCP/IP представляет адреса в числовом виде, то для перевода
	// адреса используем функцию inet_addr.
	//SAddr.sin_addr.s_addr = inet_addr((char*)"127.0.0.1");
	std::cout << "IP: " << server_ip.c_str() << "  Port: " << port << "  MaxClients: " << ClientMax << std::endl;

	inet_pton(AF_INET, (server_ip.c_str()), &SAddr.sin_addr.s_addr);
	// Порт. Используем функцию htons для перевода номера порта из обычного в //TCP/IP представление.
	SAddr.sin_port = htons(port);
	Listen = socket(AF_INET, SOCK_STREAM, 0);
	bind(Listen, (sockaddr*)& SAddr, sizeof(SAddr));

	listen(Listen, ClientMax);
	printf("Start server...\n");


	return;
}
//Извлечь данные из xml
void GetDataFromXml(std::string& server_ip, int& port)
{
	pugi::xml_document doc;
	doc.load_file("G:\\project\\Chat_Server.xml");
	pugi::xml_node chat = doc.child("chat_server");
	server_ip = chat.child("server_ip").text().as_string();
	port = chat.child("port").text().as_int();
	ClientMax = chat.child("clients").text().as_int();
}


//Прослушивание подключений к серверу
void ListenConnection()
{

	FILE* LogFile;
	fopen_s(&LogFile, "G:\\project\\logs\\ListenConnection.log", "a");
	fprintf(LogFile, "Start\n");
	fclose(LogFile);

	

	while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	{
		char buffer[100] = "";
		Sleep(50);

		fd_set s_set;
		FD_ZERO(&s_set);
		FD_SET(Listen, &s_set);
		timeval timeout = { 0, 0 };
			
		if (select(Listen + 1, &s_set, 0, 0, &timeout))
		{
			Connect = accept(Listen, NULL, NULL);
			recv(Connect, buffer, 100, NULL);

			for (auto it : names)
			{
				if (!strcmp(buffer, it.c_str()))
				{
					strcpy_s(buffer, 100, "-1");
				}
			}
			if (!strcmp(buffer, "-1"))
			{
				send(Connect, "-1", strlen("-1"), NULL);
				closesocket(Connect);
			}
			else
			{
				char Hello[200] = "";
				EnterCriticalSection(&CsConn);
				Connections.push_back(Connect);
				LeaveCriticalSection(&CsConn);

				EnterCriticalSection(&CsNames);
				names.push_back(buffer);
				LeaveCriticalSection(&CsNames);

				strcat_s(Hello, buffer);
				strcat_s(Hello, " joined the chat\r\n");
				SendMessageToClient(Hello);

				CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ConnectionClient, (LPVOID)Connect, NULL, NULL);


				fopen_s(&LogFile, "G:\\project\\logs\\ListenConnection.log", "a");
				fprintf(LogFile, "Create tread\n");
				fclose(LogFile);
			}
		}
	}

	fopen_s(&LogFile, "G:\\project\\logs\\ListenConnection.log", "a");
	fprintf(LogFile, "end\n");
	fclose(LogFile);
}


//Ф-ция для потока прослушивания клиента
void ConnectionClient(SOCKET Client)
{
	FILE* LogFile;
	fopen_s(&LogFile, "G:\\project\\logs\\Client.log", "a");
	fprintf(LogFile, "Start\n");
	fclose(LogFile);

	char buffer[1024];
	while(WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	{
		Sleep(50);
		memset(buffer, 0, sizeof(buffer));

		if (recv( Client, buffer, 1024, NULL) != SOCKET_ERROR)
		{
			SendMessageToClient(buffer);
		}
		else
		{
			auto it = std::find(Connections.begin(), Connections.end(), Client);
			strcat_s(buffer, names.at((it - Connections.begin())).c_str());
			strcat_s(buffer, " disconnected from the chat\r\n");
			SendMessageToClient(buffer);

			EnterCriticalSection(&CsNames);
			names.erase(names.begin() + (it - Connections.begin()));
			LeaveCriticalSection(&CsNames);


			EnterCriticalSection(&CsConn);
			closesocket(*it);
			Connections.erase(it);
			LeaveCriticalSection(&CsConn);


			fopen_s(&LogFile, "G:\\project\\logs\\Client.log", "a");
			fprintf(LogFile, "Client disconnect\n");
			fclose(LogFile);
			return;
		}

	}


	fopen_s(&LogFile, "G:\\project\\logs\\Client.log", "a");
	fprintf(LogFile, "end\n");
	fclose(LogFile);
	//delete []buffer;
}

//Рассылка сообщений клиентам
void SendMessageToClient(char* buffer)
{
	EnterCriticalSection(&CsConn);
	printf(buffer);
	for (SOCKET sk : Connections)
	{
		send(sk, buffer, strlen(buffer), NULL);

	}
	LeaveCriticalSection(&CsConn);
}


void StopServer()
{
	FILE* LogFile;
	fopen_s(&LogFile, "G:\\project\\logs\\Server.log", "a");
	fprintf(LogFile, "StopServer Start\n");
	fclose(LogFile);

	EnterCriticalSection(&CsConn);
	for (auto sk : Connections)
	{
		send(sk, "Server Stop\r\n", strlen("Server Stop\r\n"), NULL);
		closesocket(sk);
		Connections.erase(Connections.begin());
	}
	LeaveCriticalSection(&CsConn);


	EnterCriticalSection(&CsNames);
	names.clear();
	LeaveCriticalSection(&CsNames);

	fopen_s(&LogFile, "G:\\project\\logs\\Server.log", "a");
	fprintf(LogFile, "StopServer end\n");
	fclose(LogFile);

	closesocket(Listen);


}


void Pipes()
{
	HANDLE pSD;
	pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
		return;
	if (!SetSecurityDescriptorDacl(pSD, TRUE, NULL, FALSE))
		return;

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = FALSE;

	BOOL   fConnected = FALSE;
	DWORD  dwThreadId = 0;
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	HANDLE hThread = NULL;
	WCHAR pipename[80] = L"\\\\.\\pipe\\Server";//? Имя канала


	FILE* LogFile;
	fopen_s(&LogFile,"G:\\project\\logs\\Pipe.log", "a");
	fprintf(LogFile, "Start\n");
	fclose(LogFile);
	
	while(WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	{
		
		fopen_s(&LogFile, "G:\\project\\logs\\Pipe.log", "a");
		fprintf(LogFile, "While start\n");
		fclose(LogFile);


		hPipe = CreateNamedPipe(
			pipename,             // имя канала
			PIPE_ACCESS_DUPLEX,       // доступ на чтение\запись
			PIPE_TYPE_MESSAGE |       // Тип сообщения трубы
			PIPE_READMODE_MESSAGE |   // Режим чтения сообщение
			PIPE_WAIT,                // режим блокировки
			PIPE_UNLIMITED_INSTANCES, // max. instances
			1024,                  // Размер выходного буфера
			1024,                  // Размер входного буфера
			0,                        // Значение времени ожидания
			&sa);                    //атрибуты

		fopen_s(&LogFile, "G:\\project\\logs\\Pipe.log", "a");
		fprintf(LogFile, "hPipe created\n");
		fclose(LogFile);


		if (hPipe == INVALID_HANDLE_VALUE)
		{

			fopen_s(&LogFile, "G:\\project\\logs\\Pipe.log", "a");
			fprintf(LogFile, "Failed to create a pipe channel\n");
			fclose(LogFile);

			return;
		}

		fConnected = ConnectNamedPipe(hPipe, NULL);

		if (fConnected)
		{
			fopen_s(&LogFile, "G:\\project\\logs\\Pipe.log", "a");
			fprintf(LogFile,"PipeClient connect\n");
			fclose(LogFile);
			

			// Создаем поток для этого клиента.
			hThread = CreateThread(
				NULL,              // no security attribute
				0,                 // default stack size
				InstanceThread,    // функция обработки сообщений
				(LPVOID)hPipe,    // параметр потока
				0,                 // not suspended
				&dwThreadId);      // возврат id потока

			if (hThread == NULL)
			{
				fopen_s(&LogFile, "G:\\project\\logs\\Pipe.log", "a");
				fprintf(LogFile,"Failed to create a thread\n");
				fclose(LogFile);

				
				return;
			}

			else CloseHandle(hThread);
		}
		else
			// Ошибка подключения
			CloseHandle(hPipe);

		fopen_s(&LogFile, "G:\\project\\logs\\Pipe.log", "a");
		fprintf(LogFile, "While end\n");
		fclose(LogFile);
	}


	fopen_s(&LogFile, "G:\\project\\logs\\Pipe.log", "a");
	fprintf(LogFile, "end\n");
	fclose(LogFile);

	CloseHandle(hPipe);
}

DWORD WINAPI InstanceThread(LPVOID lpvParam)
// Эта процедура является функцией обработки нити для чтения и ответа для клиента
{
	HANDLE hHeap = GetProcessHeap();//Получаем кучу

	//TCHAR* pchRequest = (TCHAR*)HeapAlloc(hHeap, 0, 1024 * sizeof(TCHAR));//Выделяем в ней память
	//TCHAR* pchReply = (TCHAR*)HeapAlloc(hHeap, 0, 1024 * sizeof(TCHAR));//Выделяем в ней память

	DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0;
	BOOL fSuccess = FALSE;
	HANDLE hPipe = NULL;


	FILE* LogFile;
	fopen_s(&LogFile, "G:\\project\\logs\\InstanceTread.log", "a");
	fprintf(LogFile, "Start\n");
	fclose(LogFile);

	if (lpvParam == NULL)
	{
		//if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
		//if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
		return (DWORD)-1;
	}

	/*if (pchRequest == NULL)
	{
		if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
		return (DWORD)-1;
	}

	if (pchReply == NULL)
	{
		if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
		return (DWORD)-1;
	}*/

	
	fopen_s(&LogFile, "G:\\project\\logs\\InstanceTread.log", "a");
	fprintf(LogFile,"Tread create, ready  to work.\n");
	fclose(LogFile);
	

	// Параметр потоков представляет собой дескриптор экземпляра объекта трубы.
	hPipe = (HANDLE)lpvParam;

	// Цикл, пока не закончится чтение
	while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	{
		Sleep(500);
		// Читаем клиентские запросы из трубы. Это упрощенное код позволяет только только сообщения
		/*fSuccess = ReadFile(
			hPipe,        // handle to pipe
			pchRequest,    // buffer to receive data
			1024 * sizeof(TCHAR), // size of buffer
			&cbBytesRead, // number of bytes read
			NULL);        // not overlapped I/O

		if (!fSuccess || cbBytesRead == 0)
		{
			if (GetLastError() == ERROR_BROKEN_PIPE)
			{
				printf("PipeClient disconnect.\n");
			}
			else
			{
				printf("Failed to read file");
			}
			break;
		}*/

		// Обработка входящих сообщений.
		//GetAnswerToRequest(pchRequest, pchReply, &cbReplyBytes);

		// Запись ответа
		int lenght = 0;
		char namesToMesage[1000] = "";
		EnterCriticalSection(&CsNames);
		for (auto it : names)
		{
			strcat_s(namesToMesage, it.c_str());
			lenght += it.length();
			namesToMesage[lenght++] = '\n';
			namesToMesage[lenght++] = '\0';
		}
		LeaveCriticalSection(&CsNames);
		fSuccess = WriteFile(
			hPipe,        // handle to pipe
			namesToMesage,     // buffer to write from
			1000 * sizeof(char), // number of bytes to write
			&cbWritten,   // number of bytes written
			NULL);        // not overlapped I/O
		if (!fSuccess)
		{
			FILE* LogFile;
			fopen_s(&LogFile, "G:\\project\\logs\\InstanceTread.log", "a");
			fprintf(LogFile, "PipeClient Disconnect\n");
			fclose(LogFile);
			break;
		}
	}


	fopen_s(&LogFile, "G:\\project\\logs\\InstanceTread.log", "a");
	fprintf(LogFile, "End\n");
	fclose(LogFile);

	// Очищаем все переменные и закрываем канал
	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);
	//HeapFree(hHeap, 0, pchRequest);
	//HeapFree(hHeap, 0, pchReply);
	return 1;
}

VOID GetAnswerToRequest(LPTSTR pchRequest, LPTSTR pchReply, LPDWORD pchBytes)
{
	//printf("Message from PipeClient: %ws\n", pchRequest);//Это сообщение от клиента
	// Проверка исходящего сообщения, чтобы убедиться, что это не слишком длинная для буфера.
	if (FAILED(StringCchCopy(pchReply, 1024, L"Server get message )")))//Отправка ответа
	{

		*pchBytes = 0;
		pchReply[0] = 0;
		printf("Failed to get message.\n");
		return;
	}
	*pchBytes = (lstrlen(pchReply) + 1) * sizeof(TCHAR);
}
