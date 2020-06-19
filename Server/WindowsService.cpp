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
		(LPTHREAD_START_ROUTINE)Pipes,    // ������� ��������� ���������
		NULL,    // �������� ������
		0,                 // not suspended
		NULL);      // ������� id ������

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
	// ��� ������ (TCP/IP)
	SAddr.sin_family = AF_INET;
	//����� �������. �.�. TCP/IP ������������ ������ � �������� ����, �� ��� ��������
	// ������ ���������� ������� inet_addr.
	//SAddr.sin_addr.s_addr = inet_addr((char*)"127.0.0.1");
	InetPton(AF_INET, (L"127.0.0.1"), &SAddr.sin_addr.s_addr);
	// ����. ���������� ������� htons ��� �������� ������ ����� �� �������� � //TCP/IP �������������.
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
	WCHAR pipename[80] = L"\\\\.\\pipe\\Server";//? ��� ������

	// �������� ���� ������� ��������� ������������ ������ �
	// ����� ���� ������ ��� ����������� � ����. ����� ������
	// ������������, ��������� ����� ��� ��������� ���������
	// � ����� �������, � ���� ���� ����� �������� �����
	// ��������� ������ ����������� ������. ��� ����������� ����.

	for (;;)
	{
		hPipe = CreateNamedPipe(
			pipename,             // ��� ������
			PIPE_ACCESS_DUPLEX,       // ������ �� ������\������
			PIPE_TYPE_MESSAGE |       // ��� ��������� �����
			PIPE_READMODE_MESSAGE |   // ����� ������ ���������
			PIPE_WAIT,                // ����� ����������
			PIPE_UNLIMITED_INSTANCES, // max. instances
			1024,                  // ������ ��������� ������
			1024,                  // ������ �������� ������
			0,                        // �������� ������� ��������
			NULL);                    //��������

		if (hPipe == INVALID_HANDLE_VALUE)
		{
			printf("Failed to create a pipe channel\n");
			return ;
		}

		// �������� ������� ��� �����������; ���� ��� �������,
		// ������� ���������� ��������� ��������. ���� �������
		// ���������� ����, GetLastError ���������� ERROR_PIPE ����������.

		fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

		if (fConnected)
		{
			printf("PipeClient connect\n");

			// ������� ����� ��� ����� �������.
			hThread = CreateThread(
				NULL,              // no security attribute
				0,                 // default stack size
				InstanceThread,    // ������� ��������� ���������
				(LPVOID)hPipe,    // �������� ������
				0,                 // not suspended
				&dwThreadId);      // ������� id ������

			if (hThread == NULL)
			{
				printf("Failed to create a thread\n");
				return;
			}

			else CloseHandle(hThread);
		}
		else
			// ������ �����������
			CloseHandle(hPipe);
	}
}

DWORD WINAPI InstanceThread(LPVOID lpvParam)
// ��� ��������� �������� �������� ��������� ���� ��� ������ � ������ ��� �������
// � ������� ����������� � �������� ����� ���������� �� ��������� �����. �������� ��������, ��� ���������
// �������� ���� ���������� ����������, ������������ �������� ������ �������
// ���� ��������� ����� �������� ������������, � ����������� �� ���������� �����������
// ���������� ����������.
{
	HANDLE hHeap = GetProcessHeap();//�������� ����
	TCHAR* pchRequest = (TCHAR*)HeapAlloc(hHeap, 0, 1024 * sizeof(TCHAR));//�������� � ��� ������
	TCHAR* pchReply = (TCHAR*)HeapAlloc(hHeap, 0, 1024 * sizeof(TCHAR));//�������� � ��� ������

	DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0;
	BOOL fSuccess = FALSE;
	HANDLE hPipe = NULL;

	// ������� ��������� �������������� �������� � ����������� ������ ����� ���������� ��������, ���� ���� ���
	// ����� �� �������.

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

	// �������� ������� ������������ ����� ���������� ���������� ������� �����.
	hPipe = (HANDLE)lpvParam;

	// ����, ���� �� ���������� ������
	while (1)
	{
		Sleep(500);
		// ������ ���������� ������� �� �����. ��� ���������� ��� ��������� ������ ������ ���������
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
		// ��������� �������� ���������.
		//GetAnswerToRequest(pchRequest, pchReply, &cbReplyBytes);
		// ������ ������
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

	// ������� ��� ���������� � ��������� �����
	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);
	HeapFree(hHeap, 0, pchRequest);
	HeapFree(hHeap, 0, pchReply);
	return 1;
}

VOID GetAnswerToRequest(LPTSTR pchRequest, LPTSTR pchReply, LPDWORD pchBytes)
// ��� ��������� �������� ������� �������� ��� ������ ������ ������� � �������
// � ��������� �������� ����� �� ������� ������ �� ���������. ��� ��� ��
// �������� �� ������� ������� ������� ��� ���������, ������� �������� � ���������
// ���������� ������. ������ � ����, �������� ����� ����� ���������� �����
// � �������� ������ ���������� �����������, ����� ��������� ���� ��������.
{
	//printf("Message from PipeClient: %ws\n", pchRequest);//��� ��������� �� �������

	// �������� ���������� ���������, ����� ���������, ��� ��� �� ������� ������� ��� ������.
	if (FAILED(StringCchCopy(pchReply, 1024, L"Server get message )")))//�������� ������
	{
		
		*pchBytes = 0;
		pchReply[0] = 0;
		printf("Failed to get message.\n");
		return;
	}
	*pchBytes = (lstrlen(pchReply) + 1) * sizeof(TCHAR);
}
