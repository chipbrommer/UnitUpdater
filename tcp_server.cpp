///////////////////////////////////////////////////////////////////////////////
//! @file		tcp_server.cpp
//! @brief		Implementation of the tcp server class
//! @author		Chip Brommer
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
//  Includes:
//          name                        reason included
//          --------------------        ---------------------------------------
#include	"tcp_server.h"				// TCP Server Class
//
///////////////////////////////////////////////////////////////////////////////

namespace Essentials
{
	namespace Communications
	{
		const std::string TCP_Server::TcpServerVersion = "TCP Server v" +
			std::to_string(TCP_SERVER_VERSION_MAJOR) + "." +
			std::to_string(TCP_SERVER_VERSION_MINOR) + "." +
			std::to_string(TCP_SERVER_VERSION_PATCH) + " - b" +
			std::to_string(TCP_SERVER_VERSION_BUILD) + ".\n";

		std::map<TCP_Server::TcpServerError, std::string> TCP_Server::TcpServerErrorMap = {
			{ TcpServerError::NONE, std::string("Error Code " + std::to_string(static_cast<uint8_t>(TcpServerError::NONE)) + ": No error.") },
			{ TcpServerError::BAD_ADDRESS, std::string("Error Code " + std::to_string(static_cast<uint8_t>(TcpServerError::BAD_ADDRESS)) + ": Bad address.") },
			{ TcpServerError::ADDRESS_NOT_SET, std::string("Error Code " + std::to_string(static_cast<uint8_t>(TcpServerError::ADDRESS_NOT_SET)) + ": Address not set.") },
			{ TcpServerError::BAD_PORT, std::string("Error Code " + std::to_string(static_cast<uint8_t>(TcpServerError::BAD_PORT)) + ": Bad port.") },
			{ TcpServerError::PORT_NOT_SET, std::string("Error Code " + std::to_string(static_cast<uint8_t>(TcpServerError::PORT_NOT_SET)) + ": Port not set.") },
			{ TcpServerError::SERVER_ALREADY_STARTED, std::string("Error Code " + std::to_string(static_cast<uint8_t>(TcpServerError::SERVER_ALREADY_STARTED)) + ": Server already started.") },
			{ TcpServerError::FAILED_TO_CONNECT, std::string("Error Code " + std::to_string(static_cast<uint8_t>(TcpServerError::FAILED_TO_CONNECT)) + ": Failed to connect.") },
			{ TcpServerError::WINSOCK_FAILURE, std::string("Error Code " + std::to_string(static_cast<uint8_t>(TcpServerError::WINSOCK_FAILURE)) + ": Winsock creation failure.") },
			{ TcpServerError::WINDOWS_SOCKET_OPEN_FAILURE, std::string("Error Code " + std::to_string(static_cast<uint8_t>(TcpServerError::WINDOWS_SOCKET_OPEN_FAILURE)) + ": Socket open failure.") },
			{ TcpServerError::LINUX_SOCKET_OPEN_FAILURE, std::string("Error Code " + std::to_string(static_cast<uint8_t>(TcpServerError::LINUX_SOCKET_OPEN_FAILURE)) + ": Socket open failure.") },
			{ TcpServerError::ADDRESS_NOT_SUPPORTED, std::string("Error Code " + std::to_string(static_cast<uint8_t>(TcpServerError::ADDRESS_NOT_SUPPORTED)) + ": Address not supported.") },
			{ TcpServerError::BIND_FAILED, std::string("Error Code " + std::to_string(static_cast<uint8_t>(TcpServerError::BIND_FAILED)) + ": Binding to socket failed.") },
			{ TcpServerError::LISTEN_FAILED, std::string("Error Code " + std::to_string(static_cast<uint8_t>(TcpServerError::LISTEN_FAILED)) + ": Listener setup failed.") },
			{ TcpServerError::CONNECTION_FAILED, std::string("Error Code " + std::to_string(static_cast<uint8_t>(TcpServerError::CONNECTION_FAILED)) + ": Connection failed.") },
			{ TcpServerError::ACCEPT_FAILED, std::string("Error Code " + std::to_string(static_cast<uint8_t>(TcpServerError::ACCEPT_FAILED)) + ": Accepting new client failed.") },
			{ TcpServerError::ECHO_FAILED, std::string("Error Code " + std::to_string(static_cast<uint8_t>(TcpServerError::ECHO_FAILED)) + ": Echo to client failed.") },
			{ TcpServerError::RECEIVE_FAILED, std::string("Error Code " + std::to_string(static_cast<uint8_t>(TcpServerError::RECEIVE_FAILED)) + ": Receive from client failed.") }
	};

		TCP_Server::TCP_Server() : mTitle("TCP Server"), mAddress("\n"), mPort(-1), mLastError(TcpServerError::NONE)
#ifdef WIN32
			, mWsaData(), mSocket(INVALID_SOCKET)
#else
			, mSocket(-1)
#endif
		{}

		TCP_Server::TCP_Server(const std::string& address, const int port) : TCP_Server()
		{
			Configure(address, port);
		}

		int TCP_Server::Configure(const std::string& address, const int port)
		{
			if (ValidateIP(address) >= 0)
			{
				mAddress = address;
			}
			else
			{
				mLastError = TcpServerError::BAD_ADDRESS;
				return -1;
			}

			if (ValidatePort(port))
			{
				mPort = port;
			}
			else
			{
				mLastError = TcpServerError::BAD_PORT;
				return -1;
			}

			return 0;
		}

		TCP_Server::~TCP_Server()
		{
			Stop();
		}

		int TCP_Server::Start()
		{
			if (mAddress == "\n" || mAddress.empty())
			{
				mLastError = TcpServerError::ADDRESS_NOT_SET;
				return -1;
			}

			if (mPort == -1)
			{
				mLastError = TcpServerError::PORT_NOT_SET;
				return -1;
			}

#ifdef WIN32
			if (mSocket != INVALID_SOCKET)
			{
				mLastError = TcpServerError::SERVER_ALREADY_STARTED;
				return -1;
			}

			if (WSAStartup(MAKEWORD(2, 2), &mWsaData) != 0)
			{
				mLastError = TcpServerError::WINSOCK_FAILURE;
				return -1;
			}

			mSocket = socket(AF_INET, SOCK_STREAM, 0);

			if (mSocket == INVALID_SOCKET)
			{
				mLastError = TcpServerError::WINDOWS_SOCKET_OPEN_FAILURE;
				WSACleanup();
				return -1;
			}
#else
			if (mSocket != -1)
			{
				mLastError = TcpServerError::SERVER_ALREADY_STARTED;
				return -1;
			}

			mSocket = socket(AF_INET, SOCK_STREAM, 0);

			if (mSocket == -1)
			{
				mLastError = TcpServerError::LINUX_SOCKET_OPEN_FAILURE;
				return 1;
			}
#endif
			// Set up server details
			sockaddr_in serverAddress{};
			serverAddress.sin_family = AF_INET;
			serverAddress.sin_port = htons(mPort);
			if (inet_pton(AF_INET, mAddress.c_str(), &(serverAddress.sin_addr)) <= 0)
			{
				mLastError = TcpServerError::ADDRESS_NOT_SUPPORTED;
				return -1;
			}

			if (bind(mSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) 
			{
				mLastError = TcpServerError::BIND_FAILED;
				return -1;
			}

			// Listen for incoming connections
			if (listen(mSocket, SOMAXCONN) == -1)
			{
				mLastError = TcpServerError::LISTEN_FAILED;
				return -1;
			}

			// Start the monitor thread
			mStopFlag = false;
			mMonitorThread = std::thread(&TCP_Server::Monitor, this);

			return 0;
		}

		void TCP_Server::Stop()
		{
			// Set the stop flag to true to terminate the monitor thread
			mStopFlag = true;

			// Wait for the monitor thread to finish
			if (mMonitorThread.joinable()) 
			{
				mMonitorThread.join();
			}

			// Close all client sockets gracefully
			for (auto& client : mClientThreads)
			{
				//SendShutdownMessage(client); // Implement this function to send a shutdown message
				//CloseClientSocket(client);   // Implement this function to close the client socket
			}

#ifdef WIN32
			closesocket(mSocket);
			WSACleanup();
			mSocket = INVALID_SOCKET;
#else
			close(mSocket);
			mSocket = -1;
#endif
			std::cout << "Server stopped." << std::endl;
		}

		std::string TCP_Server::GetLastError()
		{
			return TcpServerErrorMap[mLastError];
		}

		int TCP_Server::ValidateIP(const std::string& ip)
		{
			// Regex expression for validating IPv4
			std::regex ipv4("(([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])");

			// Regex expression for validating IPv6
			std::regex ipv6("((([0-9a-fA-F]){1,4})\\:){7}([0-9a-fA-F]){1,4}");

			// Checking if it is a valid IPv4 addresses
			if (std::regex_match(ip, ipv4))
			{
				return 1;
			}
			// Checking if it is a valid IPv6 addresses
			else if (std::regex_match(ip, ipv6))
			{
				return 2;
			}

			// Return Invalid
			return -1;
		}

		bool TCP_Server::ValidatePort(const int port)
		{
			return (port > -1 && port < 99999);
		}

		void TCP_Server::Monitor()
		{
			while (!mStopFlag) 
			{
				sockaddr_in clientAddress{};
				socklen_t clientAddressLength = sizeof(clientAddress);
				int32_t clientSocket = accept(mSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);

				if (clientSocket == -1) 
				{
					std::cerr << "Error accepting new client: " << strerror(errno) << std::endl;
					mLastError = TcpServerError::ACCEPT_FAILED;
					continue;
				}

				// Convert client IP address to string
				char clientIP[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &(clientAddress.sin_addr), clientIP, INET_ADDRSTRLEN);

				// Print notification
				std::cout << "New client connected: " << clientIP << std::endl;

				// Handle the client connection in a separate thread
				std::thread clientThread(&TCP_Server::HandleClient, this, clientSocket, clientIP);
				mClientThreads.push_back(std::move(clientThread));

			}
		}

		void TCP_Server::HandleClient(int32_t clientSocket, const std::string& clientIP)
		{
			char buffer[1024];
			int bytesRead;

			// Receive and process client messages
			while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) > 0) 
			{
				// Null-terminate the received data
				buffer[bytesRead] = '\0';

				// Display the received message
				std::cout << "Received from client " << clientIP << ": " << buffer << std::endl;

				// Echo back the message to the client
				if (send(clientSocket, buffer, bytesRead, 0) == -1) 
				{
					mLastError = TcpServerError::ECHO_FAILED;
					break;
				}
			}

			if (bytesRead == 0) 
			{
				// Client disconnected
				std::cout << "Client disconnected: " << clientIP << std::endl;
			}
			else if (bytesRead == -1) 
			{
				// Error receiving data
				std::cerr << "Error receiving data from client " << clientIP << std::endl;
			}

#ifdef WIN32
			closesocket(clientSocket);
#else
			close(clientSocket);
#endif
		}

		// Function to send a shutdown message to all connected clients
		//void TCP_Server::SendShutdownMessageToAllClients()
		//{
		//	const char* shutdownMessage = "SERVER_SHUTDOWN";

		//	for (auto& clientThread : mClientThreads)
		//	{
		//		// Assuming each client thread is associated with a client object that has a socket member
		//		int32_t clientSocket = /* extract client socket from the associated client object */;
		//		send(clientSocket, shutdownMessage, strlen(shutdownMessage), 0);
		//	}
		//}

		//// Function to close all client sockets
		//void TCP_Server::CloseAllClientSockets()
		//{
		//	for (auto& clientThread : mClientThreads)
		//	{
		//		// Assuming each client thread is associated with a client object that has a socket member
		//		int32_t clientSocket = /* extract client socket from the associated client object */;

		//		// Close the client socket
		//		CloseClientSocket(clientSocket);
		//	}

		//	// Clear the vector of client threads
		//	mClientThreads.clear();
		//}

		// Function to send a shutdown message to the client
		int TCP_Server::SendShutdownMessage(int32_t clientSocket)
		{
			const char* shutdownMessage = "SERVER_SHUTDOWN";
			return send(clientSocket, shutdownMessage, strlen(shutdownMessage), 0);
		}

		// Function to close a client socket
		void TCP_Server::CloseClientSocket(int32_t clientSocket)
		{
#ifdef WIN32
			closesocket(clientSocket);
#else
			close(clientSocket);
#endif
		}

		void TCP_Server::CleanUpClientThreads()
		{
			for (auto it = mClientThreads.begin(); it != mClientThreads.end();) 
			{
				if (it->joinable()) 
				{
					it->join();
					it = mClientThreads.erase(it);
				}
				else 
				{
					++it;
				}
			}
		}
	}
}