#pragma comment(lib,"ws2_32.lib")
#include <WinSock2.h>
#include <iostream>
#include <WS2tcpip.h>

#include <deque>



SOCKET Connect;
//SOCKET *Connections;
std::deque<SOCKET> Connections;
SOCKET Listen;


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
			if (errno) return;
			printf("\n");
			printf(buffer);
			for (SOCKET sk : Connections)
			{
				send(sk, buffer, strlen(buffer), NULL);

			}
		}
		else
		{
			ClientCount--;
			closesocket(*std::find(Connections.begin(), Connections.end(), Client));
			Connections.erase(std::find(Connections.begin(), Connections.end(), Client));
			Connections.shrink_to_fit();
			printf("\n");
			printf("Disconnect");
			return;
		}

	}
	//delete []buffer;
}


int main()
{
	HANDLE thr;
	WSADATA data;
	WORD version = MAKEWORD(2, 2);
	int res = WSAStartup(version, &data);
	if (res != 0)
	{
		return 1;
	}
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

	SOCKET Test;
	Connections.push_back(Test);
	while (true)
	{
		Sleep(50);
		if (Connect = accept(Listen, NULL, NULL))
		{
			printf("\n");
			printf("Client connect");
			//Connections[ClientCount] = Connect;
			Connections.push_back(Connect);
			thr = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)SendMessageToClient, (LPVOID)Connect, NULL, NULL);
			ClientCount++;

		}

	}

	//delete []Connections;
	Connections.clear();
	return 0;
}