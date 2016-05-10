#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <thread>
#include "CLParser.h"


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27015
#define DEFAULT_ADDRESS "127.0.0.1"

using namespace std;

class tcp_client
{
	public:
		tcp_client(string address, int port);

		bool conn();

		bool send_data(string data) const;

		string receive() const;

		~tcp_client();

		bool IsConnected;

	private:
		SOCKET _sock;

		string _address;

		int _port;
};

tcp_client TCPClient = tcp_client(DEFAULT_ADDRESS, DEFAULT_PORT);

int g(int x)//this f ret x and calc lasts 2 s
{
	this_thread::sleep_for(std::chrono::seconds(10));
	return x;
}

int g0(int x)//ret 0 and lasts 2 sec
{
	this_thread::sleep_for(std::chrono::seconds(12));
	return 0;
}

int f(int x)//this f ret x and calc lasts 10 s
{
	this_thread::sleep_for(std::chrono::seconds(10));
	return x;
}

int fLoop(int x)//infinite loop
{
	this_thread::sleep_for(std::chrono::seconds(12));
	return 0;
}

void ConnectToServer()
{
	TCPClient.conn();
}

int __cdecl main(int argc, char **argv)
{
	CLParser parser(argc, argv, true);
	
	thread t_f(ConnectToServer);

	int* res = NULL;
	auto func = parser.get_arg("-func");
	int arg = stoi(parser.get_arg("-arg"));
	
	if (func == "g0")
	{
		res = new int(g0(arg));
	} 
	else if (func == "g")
	{
		res = new int(g(arg));
	} 
	else if (func == "f")
	{
		res = new int(f(arg));
	} 
	if (func == "fLoop")
	{
		res = new int(fLoop(arg));
	}

	t_f.join();

	if (res != NULL)
	{
		if (TCPClient.IsConnected)
			TCPClient.send_data(to_string(*res) + ";");
		else
		{
			printf("TCPClient failed to connect. Unable to send data");
		}
	}
	else
	{
		printf("Cmd Args error");
	}

	return 0;
}

tcp_client::tcp_client(string address, int port)
{
	_sock = INVALID_SOCKET;
	_port = port;
	_address = address;
	IsConnected = false;
}

bool tcp_client::conn()
{
	WSADATA wsaData;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		IsConnected = false;
		return false;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(_address.c_str(), to_string(_port).c_str(), &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		IsConnected = false;
		return false;
	}

	//server may be not started yet, so let do several attempts to connect
	for (int i = 0; i < 10 && _sock == INVALID_SOCKET; ++i)
	{
		// Attempt to connect to an address until one succeeds
		for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

			// Create a SOCKET for connecting to server
			_sock = socket(ptr->ai_family, ptr->ai_socktype,
				ptr->ai_protocol);
			if (_sock == INVALID_SOCKET) {
				printf("socket failed with error: %ld\n", WSAGetLastError());
				WSACleanup();
				IsConnected = false;
				return false;
			}

			// Connect to server.
			iResult = connect(_sock, ptr->ai_addr, (int)ptr->ai_addrlen);
			if (iResult == SOCKET_ERROR) {
				closesocket(_sock);
				_sock = INVALID_SOCKET;
				continue;
			}
			break;
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	freeaddrinfo(result);

	if (_sock == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		IsConnected = false;
		return false;
	}

	IsConnected = true;
	return true;
}

bool tcp_client::send_data(string data) const
{
	int iResult = send(_sock, data.c_str(), (int)strlen(data.c_str()), 0);
	if (iResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(_sock);
		WSACleanup();
		return false;
	}

	return true;
}

string tcp_client::receive() const
{
	const int size = 512;
	char buffer[size];
	string reply = "";

	int iResult = recv(_sock, buffer, sizeof(buffer), 0);
	if (iResult > 0)
	{
		reply = buffer;
	}
	
	return reply;
}

tcp_client::~tcp_client()
{
	closesocket(_sock);
	WSACleanup();
}
