///////////////////////////////////////////////////////////////////////////////
//! @file		tcp_server.h
//! @brief		A cross platform class to spawn a tcp server.
//! @author		Chip Brommer
///////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////
//
//  Includes:
//          name                        reason included
//          --------------------        ---------------------------------------
#ifdef WIN32
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
const int INVALID_SOCKET = -1;
const int SOCKET_ERROR = -1;
const int SD_BOTH = SHUT_RDWR;
#define closesocket(s) close(s)
#endif
#include <cstdint>						// Standard integer types
#include <map>							// Error enum to strings.
#include <string>						// Strings
#include <regex>						// Regular expression for ip validation
#include <thread>						// Multiple threads for monitor and clients. 
#include <atomic>						// Thread instance stop flag
#include <vector>						// Client thread list 
#include <iostream>						// Prints 
#include <mutex>
//
//	Defines:
//          name                        reason defined
//          --------------------        ---------------------------------------
#ifndef     CPP_TCP_SERVER				// Define the cpp tcp server class. 
#define     CPP_TCP_SERVER
//
///////////////////////////////////////////////////////////////////////////////

namespace Essentials
{
	namespace Communications
	{
		struct ClientConnection
		{
			char		ip[INET_ADDRSTRLEN] = {};
			std::int32_t	socket = 0;
			std::thread		thread;
			std::mutex		mutex;

			// Constructor for initializing the ClientConnection
			ClientConnection(int32_t sock, const std::string& clientIP) : socket(sock), thread(), mutex() 
			{
				strncpy_s(ip, clientIP.c_str(), sizeof(ip) - 1);
				ip[sizeof(ip) - 1] = '\0'; // Ensure null-termination
			}

			// Destructor to safely join the thread
			~ClientConnection() 
			{
				if (thread.joinable()) { thread.join(); }
			}

			// Move constructor
			ClientConnection(ClientConnection&& other) noexcept : socket(other.socket), thread(std::move(other.thread)), mutex() 
			{
				strncpy_s(ip, other.ip, sizeof(ip) - 1);
				ip[sizeof(ip) - 1] = '\0';
			}

			// Move assignment operator
			ClientConnection& operator=(ClientConnection&& other) noexcept 
			{
				if (this != &other) 
				{
					socket = other.socket;
					thread = std::move(other.thread);
					strncpy_s(ip, other.ip, sizeof(ip) - 1);
					ip[sizeof(ip) - 1] = '\0';
				}

				return *this;
			}
		};

		class TCP_Server
		{
		public:
			static constexpr int TCP_SERVER_VERSION_MAJOR = 0;
			static constexpr int TCP_SERVER_VERSION_MINOR = 1;
			static constexpr int TCP_SERVER_VERSION_PATCH = 0;
			static constexpr int TCP_SERVER_VERSION_BUILD = 0;

			static const std::string TcpServerVersion;

			/// <summary>enum for error codes</summary>
			enum class TcpServerError : uint8_t
			{
				NONE,
				BAD_ADDRESS,
				ADDRESS_NOT_SET,
				BAD_PORT,
				PORT_NOT_SET,
				SERVER_ALREADY_STARTED,
				FAILED_TO_CONNECT,
				WINSOCK_FAILURE,
				WINDOWS_SOCKET_OPEN_FAILURE,
				LINUX_SOCKET_OPEN_FAILURE,
				ADDRESS_NOT_SUPPORTED,
				BIND_FAILED,
				LISTEN_FAILED,
				CONNECTION_FAILED,
				ACCEPT_FAILED,
				ECHO_FAILED,
				RECEIVE_FAILED,
			};

			/// <summary>Error enum to string map</summary>
			static std::map<TcpServerError, std::string> TcpServerErrorMap;

			/// <summary>Default constructor</summary>
			TCP_Server();

			/// <summary>Constructor with default parameters</summary>
			/// <param name="address"></param>
			/// <param name="port"></param>
			explicit TCP_Server(const std::string& address, const int port);

			/// <summary>Default deconstructor</summary>
			~TCP_Server();

			/// <summary>Configures the server to a desired address and port number</summary>
			/// <param name="address"> -[in]- Address to serve the TCP server on.</param>
			/// <param name="port"> -[in]- Port to serve the TCP server on.</param>
			/// <returns>0 if successful, -1 if fails. Call Serial::GetLastError to find out more.</returns>
			int Configure(const std::string& address, const int port);

			/// <summary>Starts the TCP server.</summary>
			/// <returns>0 if successful, -1 if fails. Call Serial::GetLastError to find out more.</returns>
			int Start();

			/// <summary>Stops the TCP server if it is running and cleans up the monitor thread.</summary>
			void Stop();

			/// <summary>Get the last error in string format</summary>
			/// <returns>The last error in a formatted string</returns>
			std::string GetLastError();

		protected:
		private:
			/// <summary>Validates an IP address is IPv4 or IPv6</summary>
			/// <param name="ip"> -[in]- IP Address to be validated</param>
			/// <returns>-1 = bad IP, 1 = valid IPv4, 2 = valid IPv6</returns>
			int ValidateIP(const std::string& ip);

			/// <summary>Validates a port number is between 0-99999</summary>
			/// <param name="port"> -[in]- Port number to be validated</param>
			/// <returns>true = valid, false = invalid</returns>
			bool ValidatePort(const int port);

			/// <summary>Starts a thread to monitor the TCP connection.</summary>
			void Monitor();

			/// <summary>Handles a client connection to the server</summary>
			/// <param name="clientSocket"> -[in]- socket descriptor for the client</param>
			/// <param name="clientIP"> -[in]- address of the client</param>
			void HandleClient(int32_t clientSocket, const std::string& clientIP);

			int SendShutdownMessage(int32_t clientSocket);

			/// @brief Close a single client connection
			void CloseClientSocket(int32_t clientSocket);
			
			/// @brief Close all client sockets in mClient vector
			void CloseAllClientSockets();

			/// @brief Clean up all thread in mClients vector
			void CleanUpClientThreads();

			std::string mTitle;					// Title of the class - used when using CPP_LOGGER
			std::string mAddress;				// Address of the TCP server
			int mPort;							// Port of the TCP server
			TcpServerError mLastError;			// Holds last error of the TCP server
			std::thread mMonitorThread;			// Holds the thread for the monitor. 
			std::atomic<bool> mStopFlag;		// Stop flag for the server. 
			std::vector<ClientConnection> mClients;	

#ifdef WIN32
			WSADATA mWsaData;
#endif
			SOCKET mSocket;
		};
	} // Communications

} // Essentials

#endif // CPP_TCP_SERVER
