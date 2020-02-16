// cmd-chat.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

bool DEBUG = false;

class NetworkWorker
{
public:
	NetworkWorker() :
		m_Socket(INVALID_SOCKET),
		m_Port(DEFAULT_PORT)
	{
	}
	~NetworkWorker()
	{
		closesocket(m_Socket);
		WSACleanup();
	}

	bool Init()
	{
		if (DEBUG) std::cout << "Init.. " << std::endl;
		WSADATA wsaData;
		int iResult = 0;

		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != NO_ERROR)
		{
			if (DEBUG) std::cout << "Initialization failed with error: " << iResult << std::endl;
			return false;
		}
		return true;
	}

	bool CreateSocket()
	{
		if (DEBUG) std::cout << "Create Socket" << std::endl;

		m_Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (m_Socket == INVALID_SOCKET)
		{
			if (DEBUG) std::cout << "Creating socket failed with error: " << WSAGetLastError() << std::endl;
			return false;
		}

		if (DEBUG) std::cout << "Set broadcast opt" << std::endl;
		const char broadcastOptionValue = '1';
		if (setsockopt(m_Socket, SOL_SOCKET, SO_BROADCAST, &broadcastOptionValue, sizeof(broadcastOptionValue)) < 0)
		{
			if (DEBUG) std::cout << "Error in setting Broadcast option" << std::endl;
			return false;
		}

		//  Put a socket in non-blocking mode
		DWORD nonBlocking = 1;
		if (ioctlsocket(m_Socket, FIONBIO, &nonBlocking) != 0)
		{
			if (DEBUG) std::cout << "Failed to set non-blocking" << std::endl;
			return false;
		}

		return true;
	}

	bool Bind()
	{
		if (DEBUG) std::cout << "Bind.." << std::endl;

		bool isSuccess = false;
		sockaddr_in RecvAddr;
		RecvAddr.sin_family = AF_INET;
		RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		for (m_Port = DEFAULT_PORT; m_Port < DEFAULT_PORT + USERS_MAX_NUMBER; ++m_Port)
		{
			RecvAddr.sin_port = htons(m_Port);
			int iResult = bind(m_Socket, (SOCKADDR*)& RecvAddr, sizeof(RecvAddr));
			if (iResult != 0)
			{
				if (DEBUG) std::cout << "bind failed with error: " << WSAGetLastError() << std::endl;
				if (DEBUG) std::cout << "Try again..." << std::endl;
				continue;
			}
			if (DEBUG) std::cout << "Success: " << m_Port << std::endl;
			isSuccess = true;
			break;
		}
		return isSuccess;
	}

	void Send(std::string const& mess)
	{
		if (DEBUG) std::cout << "Send.." << std::endl;

		struct sockaddr_in BroadcastAddress;
		BroadcastAddress.sin_family = AF_INET;
		BroadcastAddress.sin_addr.s_addr = INADDR_BROADCAST;

		for (unsigned short i = DEFAULT_PORT; i < DEFAULT_PORT + USERS_MAX_NUMBER; ++i)
		{
			if (i == m_Port)
			{
				continue;
			}
			BroadcastAddress.sin_port = htons(i);
			int iResult = sendto(m_Socket, mess.c_str(), mess.length() + 1, 0, (sockaddr*)& BroadcastAddress, sizeof(BroadcastAddress));
			if (iResult == SOCKET_ERROR)
			{
				if (DEBUG) std::cout << "Send failed: " << WSAGetLastError() << std::endl;
				return;
			}
		}
	}

	void Receive()
	{
		if (DEBUG) std::cout << "Receiving datagrams..." << std::endl;

		char RecvBuf[1024];
		int BufLen = 1024;
		sockaddr_in SenderAddr;
		int SenderAddrSize = sizeof(SenderAddr);

		int iResult = recvfrom(m_Socket, RecvBuf, BufLen, 0, (SOCKADDR*)& SenderAddr, &SenderAddrSize);
		if (iResult == SOCKET_ERROR)
		{
			return;
		}

		if (iResult > 0)
		{
			if (DEBUG) std::cout << "Bytes received: " << iResult << std::endl;
			std::cout << "\n\t Received message =>  " << RecvBuf << std::endl;
		}
		else if (iResult == 0)
		{
			std::cout << "Connection closed" << iResult << std::endl;
			return;
		}
	}
private:

	unsigned short const DEFAULT_PORT = 12121;
	unsigned short const USERS_MAX_NUMBER = 10;

	SOCKET m_Socket;
	unsigned short m_Port;
};

int main()
{
	std::cout << ">>>>>  You are welcome to MiniChatik  <<<<<<" << std::endl;

	NetworkWorker socketIns;

	if (!socketIns.Init())
	{
		std::cout << "Something went wrong ((" << std::endl;
		return 1;
	}
	if (!socketIns.CreateSocket())
	{
		std::cout << "Something went wrong ((" << std::endl;
		return 1;
	}
	if (!socketIns.Bind())
	{
		std::cout << "Something went wrong ((" << std::endl;
		return 1;
	}

	bool isFinish = false;

	auto sendFun = [&socketIns, &isFinish]()
	{
		while (!isFinish)
		{
			std::cout << "Enter (q) to quit, (m) to write message: " << std::endl;
			std::string mess;
			std::cin >> mess;
			if (mess == "m")
			{
				std::cout << "Enter message: " << std::endl;
				std::cin >> mess;

				socketIns.Send(mess);
			}
			else if (mess == "q")
			{
				isFinish = true;
			}
			else
			{
				std::cout << "Try again..." << std::endl;
			}
		}
	};
	std::thread sendThread(sendFun);

	while (!isFinish)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		socketIns.Receive();
	}

	sendThread.join();

	std::cout << "<<<<< BYE >>>>>" << std::endl;
	return 0;

}