#pragma comment(lib,"ws2_32.lib")
#include <WinSock2.h>
#include <iostream>
#include <WS2tcpip.h>

#include <strsafe.h>
#include <deque>


void Connection();
void ConnectionClient(SOCKET);
void SendMessageToClient(char*);
void Pipes();
DWORD WINAPI InstanceThread(LPVOID);
VOID GetAnswerToRequest(LPTSTR, LPTSTR , LPDWORD);


SOCKET Connect;
std::deque<SOCKET> Connections;
std::deque<std::string>names;
SOCKET Listen;
CRITICAL_SECTION CsConn;
CRITICAL_SECTION CsNames;


int ClientMax = 100;

int main()
{

	InitializeCriticalSection(&CsConn);
	InitializeCriticalSection(&CsNames);

	CreateThread(
		NULL,              // no security attribute
		0,                 // default stack size
		(LPTHREAD_START_ROUTINE)Pipes,    // функция обработки сообщений
		NULL,    // параметр потока
		0,                 // not suspended
		NULL);      // возврат id потока

	Connection();

	while (true)
	{
		
		char buffer[100] ="";
		Sleep(50);
		if (Connect = accept(Listen, NULL, NULL))
		{
			recv(Connect, buffer, 100, NULL);

			for (auto it : names)
			{
				if (!strcmp(buffer, it.c_str()))
				{
					strcpy_s(buffer,100 ,"-1");
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
				strcat_s(Hello, " joined the chat");
				SendMessageToClient(Hello);

				CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ConnectionClient, (LPVOID)Connect, NULL, NULL);
			}
		}

	}

	names.clear();
	Connections.clear();
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

	sockaddr_in SAddr;
	ZeroMemory(&SAddr, sizeof(SAddr));
	// тип адреса (TCP/IP)
	SAddr.sin_family = AF_INET;
	//адрес сервера. Т.к. TCP/IP представляет адреса в числовом виде, то для перевода
	// адреса используем функцию inet_addr.
	//SAddr.sin_addr.s_addr = inet_addr((char*)"127.0.0.1");
	InetPton(AF_INET, (L"127.0.0.1"), &SAddr.sin_addr.s_addr);
	// Порт. Используем функцию htons для перевода номера порта из обычного в //TCP/IP представление.
	SAddr.sin_port = htons(8488);
	Listen = socket(AF_INET, SOCK_STREAM, 0);
	bind(Listen, (sockaddr*)& SAddr, sizeof(SAddr));

	listen(Listen, ClientMax);
	printf("Start server...\n");
	return;
}

void ConnectionClient(SOCKET Client)
{
	char buffer[1024];
	for (;; Sleep(50))
	{

		memset(buffer, 0, sizeof(buffer));

		if (recv(*std::find(Connections.begin(), Connections.end(), Client), buffer, 1024, NULL) != SOCKET_ERROR)
		{
			SendMessageToClient(buffer);
		}
		else
		{
			auto it = std::find(Connections.begin(), Connections.end(), Client);
			strcat_s(buffer, names.at((it - Connections.begin())).c_str());
			strcat_s(buffer, " disconnected from the chat");
			SendMessageToClient(buffer);

			EnterCriticalSection(&CsNames);
			names.erase(names.begin() + (it - Connections.begin()));
			LeaveCriticalSection(&CsNames);


			EnterCriticalSection(&CsConn);
			closesocket(*it);
			Connections.erase(it);
			LeaveCriticalSection(&CsConn);

			return;
		}

	}
	//delete []buffer;
}


void SendMessageToClient(char* buffer)
{
	EnterCriticalSection(&CsConn);
	printf(buffer);
	printf("\n");
	for (SOCKET sk : Connections)
	{
		send(sk, buffer, strlen(buffer), NULL);

	}
	LeaveCriticalSection(&CsConn);
}


void Pipes()
{
	BOOL   fConnected = FALSE;
	DWORD  dwThreadId = 0;
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	HANDLE hThread = NULL;
	WCHAR pipename[80] = L"\\\\.\\pipe\\Server";//? Имя канала

	// Основной цикл создает экземпляр именованного канала и
	// Затем ждет клиент для подключения к нему. Когда клиент
	// Подключается, создается поток для обработки сообщений
	// С этого клиента, и этот цикл может свободно ждать
	// Следующий клиент подключения запрос. Это бесконечный цикл.

	for (;;)
	{
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
			NULL);                    //атрибуты

		if (hPipe == INVALID_HANDLE_VALUE)
		{
			printf("Failed to create a pipe channel\n");
			return ;
		}

		// Ожидание клиента для подключения; если это удастся,
		// функция возвращает ненулевое значение. Если функция
		// возвращает ноль, GetLastError возвращает ERROR_PIPE подключены.

		fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

		if (fConnected)
		{
			printf("PipeClient connect\n");

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
				printf("Failed to create a thread\n");
				return;
			}

			else CloseHandle(hThread);
		}
		else
			// Ошибка подключения
			CloseHandle(hPipe);
	}
}

DWORD WINAPI InstanceThread(LPVOID lpvParam)
// Эта процедура является функцией обработки нити для чтения и ответа для клиента
// С помощью подключения к открытой трубе передается от основного цикла. Обратите внимание, это позволяет
// Основной цикл продолжить выполнение, потенциально создавая больше потоков
// Этой процедуры могут работать одновременно, в зависимости от количества поступающих
// Клиентские соединения.
{
	HANDLE hHeap = GetProcessHeap();//Получаем кучу
	TCHAR* pchRequest = (TCHAR*)HeapAlloc(hHeap, 0, 1024 * sizeof(TCHAR));//Выделяем в ней память
	TCHAR* pchReply = (TCHAR*)HeapAlloc(hHeap, 0, 1024 * sizeof(TCHAR));//Выделяем в ней память

	DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0;
	BOOL fSuccess = FALSE;
	HANDLE hPipe = NULL;

	// Сделать некоторые дополнительные проверки с приложением ошибку будет продолжать работать, даже если это
	// поток не удается.

	if (lpvParam == NULL)
	{
		printf("\nERROR - Pipe Server Failure:\n");
		printf("   InstanceThread got an unexpected NULL value in lpvParam.\n");
		printf("   InstanceThread exitting.\n");
		if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
		if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
		return (DWORD)-1;
	}

	if (pchRequest == NULL)
	{
		printf("\nERROR - Pipe Server Failure:\n");
		printf("   InstanceThread got an unexpected NULL heap allocation.\n");
		printf("   InstanceThread exitting.\n");
		if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
		return (DWORD)-1;
	}

	if (pchReply == NULL)
	{
		printf("\nERROR - Pipe Server Failure:\n");
		printf("   InstanceThread got an unexpected NULL heap allocation.\n");
		printf("   InstanceThread exitting.\n");
		if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
		return (DWORD)-1;
	}

	printf("Tread create, ready  to work.\n");

	// Параметр потоков представляет собой дескриптор экземпляра объекта трубы.
	hPipe = (HANDLE)lpvParam;

	// Цикл, пока не закончится чтение
	while (1)
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
		EnterCriticalSection(&CsNames);
		int lenght = 0;
		char namesToMesage[1000]="";
		for (auto it : names)
		{
			strcat_s(namesToMesage, it.c_str());
			lenght += it.length();
			namesToMesage[lenght++] = '\n';
			namesToMesage[lenght++] = '\0';
		}
		fSuccess = WriteFile(
			hPipe,        // handle to pipe
			namesToMesage,     // buffer to write from
			1000*sizeof(char), // number of bytes to write
			&cbWritten,   // number of bytes written
			NULL);        // not overlapped I/O
		LeaveCriticalSection(&CsNames);
		if (!fSuccess)
		{
			printf("PipeClient Disconnect\n");
			break;
		}
	}

	// Очищаем все переменные и закрываем канал
	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);
	HeapFree(hHeap, 0, pchRequest);
	HeapFree(hHeap, 0, pchReply);
	return 1;
}

VOID GetAnswerToRequest(LPTSTR pchRequest, LPTSTR pchReply, LPDWORD pchBytes)
// Эта процедура является простой функцией для печати запрос клиента к консоли
// И заполнить ответный буфер со строкой данных по умолчанию. Это где вы
// Поставил бы реально запроса клиента код обработки, которая работает в контексте
// Экземпляра потока. Имейте в виду, основной поток будет продолжать ждать
// И получить другие клиентские подключения, когда экземпляр нить работает.
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
