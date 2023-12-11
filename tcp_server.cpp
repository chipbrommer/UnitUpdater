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

		TCP_Server::TCP_Server() : mMaxClients(FD_SETSIZE), mAddress("\n"), mPort(-1), mLastError(TcpServerError::NONE)
#ifdef WIN32
			, mWsaData(), mSocket(INVALID_SOCKET)
#else
			, mSocket(-1)
#endif
		{}

		TCP_Server::TCP_Server(int maxClients) : mMaxClients(maxClients), mAddress("\n"), mPort(-1), mLastError(TcpServerError::NONE)
#ifdef WIN32
			, mWsaData(), mSocket(INVALID_SOCKET)
#else
			, mSocket(-1)
#endif
		{}

		TCP_Server::TCP_Server(const int maxClients, const std::string& address, const int port) : TCP_Server(maxClients)
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
#ifdef WIN32
			FD_ZERO(&mFDs);
#else
			delete[] mFDs;
#endif
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
#ifdef WIN32
			FD_ZERO(&mFDs);
			FD_SET(mSocket, &mFDs);
#else
			memset(mFDs, 0, sizeof(mFDs));
#endif
			mStopFlag = false;
			return 0;
		}

		int TCP_Server::Run()
		{
#ifdef WIN32
			fd_set reads;
			reads = mFDs;
#else

#endif

			// default return 
			return -1;
		}

		void TCP_Server::Stop()
		{
			// Set the stop flag
			mStopFlag = true;

			CloseAllClientSockets();

#ifdef WIN32
			closesocket(mSocket);
			WSACleanup();
			mSocket = INVALID_SOCKET;
#else
			close(mSocket);
			mSocket = -1;
#endif
			std::cout << "[SERVER] Stopped." << std::endl;
		}

		int TCP_Server::SendMessageToClient(const int clientFD, const uint8_t* msg)
		{
			return -1;
		}

		std::string TCP_Server::GetLastError()
		{
			return TcpServerErrorMap[mLastError];
		}

		void TCP_Server::SetConnectionCallback(const std::function<int(const int)>& handler)
		{
			mNewConnectionHandler = handler;
		}

		void TCP_Server::SetMessageCallback(const std::function<int(const int, const std::string&)>& handler)
		{
			mMessageHandler = handler;
		}

		void TCP_Server::SetDisconnectCallback(const std::function<int(const int)>& handler)
		{
			mDisconnectHandler = handler;
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

		void TCP_Server::CloseAllClientSockets()
		{
			for (auto& client : mClients)
			{
				std::lock_guard<std::mutex> clientLock(client.mutex);
				SendShutdownMessage(client.socket);
				CloseClientSocket(client.socket);
			}
		}

		int TCP_Server::SendShutdownMessage(int32_t clientSocket)
		{
			const char* shutdownMessage = "SERVER_SHUTDOWN";
			return send(clientSocket, shutdownMessage, strlen(shutdownMessage), 0);
		}

		void TCP_Server::CloseClientSocket(int32_t clientSocket) 
		{
#ifdef _WIN32
			shutdown(clientSocket, SD_BOTH);
			closesocket(clientSocket);
#else
			shutdown(clientSocket, SHUT_RDWR);
			close(clientSocket);
#endif
		}
	
	}
}