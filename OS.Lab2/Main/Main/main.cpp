//#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>
#include <thread>
#include <iostream>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#define NON_BLOCKING (u_long)1
#define BLOCKING (u_long)0

struct Client
{
	SOCKET Sock;
	int* Result;
	bool Opened;

	Client(SOCKET sock) : Sock(sock), Result(NULL), Opened(false) {};
	
	bool Open(SOCKET ListenSocket);
	
	void Close();
	
	int Receive(SOCKET ListenSocket);
	
	bool HasValue() const
	{ return Result != NULL; }
	
	int Value() const
	{ return *Result; }
};

SOCKET InitializeWinSock();

bool Block(SOCKET sock);

bool UnBlock(SOCKET sock);

void OpenClient(Client cli, SOCKET sock);

struct Process
{
	PROCESS_INFORMATION ProcessInfo;
	bool Started;

	Process(const LPCWSTR path, const LPWSTR args);
	void Close() const;
};

Process::Process(const LPCWSTR path, const LPWSTR args)
{
	STARTUPINFO si;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&ProcessInfo, sizeof(ProcessInfo));

	auto res = CreateProcessW(path,
		args, NULL, NULL, FALSE, 0, NULL, NULL, &si, &ProcessInfo);

	if (!res)
	{
		printf("Failed to start process");
		Started = false;
	}

	Started = true;
}

void Process::Close() const
{
	TerminateProcess(ProcessInfo.hProcess, 0);
}

int main()
{
	Client client1 = INVALID_SOCKET;
	Client client2 = INVALID_SOCKET;
		
	const LPCWSTR path1 = TEXT("..\\x64\\Debug\\Function1.exe");
	const LPCWSTR path2 = TEXT("..\\x64\\Debug\\Function1.exe");

	const LPWSTR args1 = L"-func fLoop -arg 3";
	const LPWSTR args2 = L"-func g0 -arg 4";

	auto proc1 = Process(path1, args1);
	auto proc2 = Process(path2, args2);
		
	SOCKET ListenSocket = InitializeWinSock();

	if (ListenSocket == NULL)
	{
		return 1;
	}
	
	bool ask = true;
	auto last_asked_time = std::chrono::steady_clock::now();
	while (true) {
		if (!client1.HasValue())
			client1.Receive(ListenSocket);
		
		if (!client2.HasValue())
			client2.Receive(ListenSocket);

		if (client1.HasValue() && client1.Value() == 0 || 
			client2.HasValue() && client2.Value() == 0) 
		{
			std::cout << "0" << std::endl;
			break;
		}
		if (client1.HasValue() && client2.HasValue()) {
			std::cout << client1.Value()*client2.Value() << std::endl;
			break;
		}
		if (ask && std::chrono::steady_clock::now() - last_asked_time > std::chrono::seconds(5)) {
			std::cout << "What you wish to do?\n";
			std::cout << "\t1) Continue.\n";
			std::cout << "\t2) Break.\n";
			std::cout << "\t3) Never ask again\n";
			int choice;
			std::cin >> choice;
			if (choice == 2) exit(0);
			if (choice == 3) ask = false;
			last_asked_time = std::chrono::steady_clock::now();
		}
	}

	proc1.Close();
	proc2.Close();

	closesocket(ListenSocket);

	client1.Close();
	client2.Close();

	WSACleanup();

	system("pause");
	return 0;
}

bool Client::Open(SOCKET ListenSocket)
{
	if (Opened)
		return true;

	// Accept a client socket
	Sock = accept(ListenSocket, NULL, NULL);
	if (Sock == INVALID_SOCKET) {
		Opened = false;
		return false;
	}

	if (!UnBlock(Sock))
	{
		Opened = false;
		return false;
	}

	Opened = true;
	return true;
}

void Client::Close()
{
	if (!Opened)
		return;

	// shutdown the connection since we're done
	if (shutdown(Sock, SD_SEND) == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
	}

	// cleanup
	closesocket(Sock);

	Opened = false;
}

int Client::Receive(SOCKET ListenSocket)
{
	if (!Opened)
	{
		if (!Open(ListenSocket))
		{
			return 0;
		}
	}

	//if we already have value than we don't need to receive anything
	if (HasValue())
		return 0;

	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	int iResult = recv(Sock, recvbuf, recvbuflen, 0);
	if (iResult > 0) {
		std::string res(recvbuf, 0, recvbuflen);
		std::string number = res.substr(0, res.find(";"));

		Result = new int(std::stoi(number));

		// Echo the buffer back to the sender
		int iSendResult = send(Sock, recvbuf, iResult, 0);
		if (iSendResult == SOCKET_ERROR)
		{
//			printf(" send failed with error: %d\n", WSAGetLastError());
			Close();
		}
	}
	return iResult;
}

bool TurnIOCTL(SOCKET sock, u_long mode)
{
	int unBlock = ioctlsocket(sock, FIONBIO, &mode);
	if (unBlock != NO_ERROR)
	{
		printf("unblocking failed: %d\n", WSAGetLastError());
		closesocket(sock);
		WSACleanup();
		return false;
	}
	return true;
}

bool Block(SOCKET sock)
{
	return TurnIOCTL(sock, BLOCKING);
}

bool UnBlock(SOCKET sock)
{
	return TurnIOCTL(sock, NON_BLOCKING);
}

SOCKET InitializeWinSock()
{
	WSADATA wsaData;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	// Initialize Winsock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return NULL;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return NULL;
	}

	// Create a SOCKET for connecting to server
	SOCKET ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return NULL;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return NULL;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return NULL;
	}

	/* set socket to non-blocking i/o */
	if (!UnBlock(ListenSocket))
		return NULL;

	return ListenSocket;
}

void OpenClient(Client cli, SOCKET sock)
{
	while (!cli.Open(sock));
}
