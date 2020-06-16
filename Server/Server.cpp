#pragma comment(lib,"ws2_32.lib")
#include <WinSock2.h>
#include <iostream>
#include <WS2tcpip.h>

#include <deque>



SOCKET Connect;
//SOCKET *Connections;
std::deque<SOCKET> Connections;
std::deque<std::string>names;
SOCKET Listen;
CRITICAL_SECTION CsConn;
CRITICAL_SECTION CsNames;

int ClientCount = 0;
int ClientMax = 100;

void SendMessageToClient(SOCKET Client)
{
	char buffer[1024];
	for (;; Sleep(50))
	{

		

		memset(buffer, 0, sizeof(buffer));
		
		if (recv(*std::find(Connections.begin(), Connections.end(), Client), buffer, 1024, NULL) != SOCKET_ERROR)
		{
			EnterCriticalSection(&CsConn);
			printf("\n");
			printf(buffer);
			for (SOCKET sk : Connections)
			{
				send(sk, buffer, strlen(buffer), NULL);

			}
			LeaveCriticalSection(&CsConn);
		}
		else
		{
			EnterCriticalSection(&CsConn);
			ClientCount--;
			auto it = std::find(Connections.begin(), Connections.end(), Client);
			closesocket(*it);
			names.erase(names.begin() + (it - Connections.begin()));
			Connections.erase(it);
			LeaveCriticalSection(&CsConn);
			printf("\n");
			printf("Disconnect");
			return;
		}

	}
	//delete []buffer;
}


int main()
{
	WSADATA data;
	WORD version = MAKEWORD(2, 2);
	if (WSAStartup(version, &data))
	{
		return 1;
	}
	InitializeCriticalSection(&CsConn);


	/*   можно заменить
	if (FAILED (WSAStartup (MAKEWORD( 2, 2 ), &data) ) )
	{
	  // Error...
	  error = WSAGetLastError();
	  //...
	}
	*/

	//Connections = new SOCKET[ClientMax];

	// С сайта https://club.shelek.ru/viewart.php?id=35
	sockaddr_in SAddr;
	ZeroMemory(&SAddr, sizeof(SAddr));
	// тип адреса (TCP/IP)
	SAddr.sin_family = AF_INET;
	//адрес сервера. Т.к. TCP/IP представляет адреса в числовом виде, то для перевода
	// адреса используем функцию inet_addr.
	//SAddr.sin_addr.s_addr = inet_addr((char*)"127.0.0.1");
	InetPton(AF_INET, ("127.0.0.1"), &SAddr.sin_addr.s_addr);
	// Порт. Используем функцию htons для перевода номера порта из обычного в //TCP/IP представление.
	SAddr.sin_port = htons(8488);
	Listen = socket(AF_INET, SOCK_STREAM, 0);
	bind(Listen, (sockaddr*)& SAddr, sizeof(SAddr));


	// Из видео
	/*
	addrinfo hints, *rezult;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	GetAddrInfo(NULL, "7700", &hints, &rezult);
	Listen = socket(rezult->ai_family, rezult->ai_socktype, rezult->ai_protocol);
	bind(Listen, rezult->ai_addr, rezult->ai_addrlen);
	freeaddrinfo(rezult);
	*/

	listen(Listen, ClientMax);
	printf("Start server...");
	
	while (true)
	{
		char Hello[200] = "HEllo ";
		char buffer[100];
		Sleep(50);
		if (Connect = accept(Listen, NULL, NULL))
		{
			printf("\n");
			printf("Client connect");
			//Connections[ClientCount] = Connect;
			EnterCriticalSection(&CsConn);
			recv(Connect, buffer, 100, NULL);
			Connections.push_back(Connect);
			names.push_back(buffer);
			strcat_s(Hello, 100, buffer);
			send(Connect, Hello, strlen(Hello), NULL);
			LeaveCriticalSection(&CsConn);
			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)SendMessageToClient, (LPVOID)Connect, NULL, NULL);
			ClientCount++;

		}

	}

	//delete []Connections;
	Connections.clear();
	return 0;
}