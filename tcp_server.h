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
#ifndef     TCP_SERVER					// Define the cpp tcp server class. 
#define     TCP_SERVER
//
///////////////////////////////////////////////////////////////////////////////

namespace Essentials
{
	namespace Communications
	{
		struct Client
		{
			std::string ip;					// ip address of the client
			std::uint16_t port;				// port of the client
			std::int32_t socket;			// socket of the client
			std::int32_t bytesReceived;		// number of bytes received from the client
			std::int32_t bytesWritten;		// number of bytes written to the client
			std::int32_t timeConnected;		// the timestamp when the client was connected to the server
			std::mutex mutex;				// data safety mutex

			// Default constructor
			Client() : socket(-1), bytesReceived(0), bytesWritten(0), timeConnected(0) {}

			// Constructor with parameters
			Client(const std::string& _ip, const std::uint16_t _port, std::int32_t _socket, std::int32_t _bytesReceived,
				std::int32_t _bytesWritten, std::int32_t _timeConnected)
				: ip(_ip), port(_port), socket(_socket), bytesReceived(_bytesReceived),
				bytesWritten(_bytesWritten), timeConnected(_timeConnected) {}

			// Copy constructor
			Client(const Client& other)
				: ip(other.ip), port(other.port), socket(other.socket), bytesReceived(other.bytesReceived),
				bytesWritten(other.bytesWritten), timeConnected(other.timeConnected) {}

			// Assignment operator
			Client& operator=(const Client& other) 
			{
				if (this != &other) 
				{
					ip				= other.ip;
					port			= other.port;
					socket			= other.socket;
					bytesReceived	= other.bytesReceived;
					bytesWritten	= other.bytesWritten;
					timeConnected	= other.timeConnected;
				}
				return *this;
			}

			// Move constructor
			Client(Client&& other) noexcept
				: ip(std::move(other.ip)), port(other.port), socket(other.socket), bytesReceived(other.bytesReceived),
				bytesWritten(other.bytesWritten), timeConnected(other.timeConnected) {}

			// Move assignment operator
			Client& operator=(Client&& other) noexcept 
			{
				if (this != &other) 
				{
					// Copy the data over
					ip					= std::move(other.ip);
					port				= other.port;
					socket				= other.socket;
					bytesReceived		= other.bytesReceived;
					bytesWritten		= other.bytesWritten;
					timeConnected		= other.timeConnected;
				}
				return *this;
			}

			// Equality operator
			bool operator==(const Client& other) const 
			{
				return	ip				== other.ip				&&
						port			== other.port			&&
						socket			== other.socket			&&
						bytesReceived	== other.bytesReceived	&&
						bytesWritten	== other.bytesWritten	&&
						timeConnected	== other.timeConnected;
			}

			// Inequality operator
			bool operator!=(const Client& other) const 
			{
				return !(*this == other);
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

			/// @brief enum for error codes
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

			/// @brief Error enum to string map
			static std::map<TcpServerError, std::string> TcpServerErrorMap;

			/// @brief Default constructor
			TCP_Server();

			/// @brief Constructor with default parameters
			/// @param address - in - Address to serve the server on.
			/// @param port - in - Port to serve the server on.
			explicit TCP_Server(const std::string& address, const int port);

			/// @brief Default deconstructor
			~TCP_Server();

			/// @brief Configures the server to a desired address and port number
			/// @param address - in - Address to serve the server on.
			/// @param port - in - Port to serve the server on.
			/// @return 0 if successful, -1 if fails. Call Serial::GetLastError to find out more.
			int Configure(const std::string& address, const int port);

			/// @brief Starts the server.
			/// @return 0 if successful, -1 if fails. Call Serial::GetLastError to find out more.
			int Start();

			/// @brief Stops the server if it is running and cleans up the monitor thread.
			void Stop();

			/// @brief Get the last error in string format
			/// @return The last error in a formatted string
			std::string GetLastError();

		protected:
		private:
			/// @brief Validates an IP address is IPv4 or IPv6
			/// @param ip - in - IP Address to be validated
			/// @return -1 = bad IP, 1 = valid IPv4, 2 = valid IPv6
			int ValidateIP(const std::string& ip);

			/// @brief Validates a port number is between 0-99999
			/// @param port - in - Port number to be validated
			/// @return true = valid, false = invalid
			bool ValidatePort(const int port);

			/// @brief Starts a thread to monitor the TCP connection.
			void Monitor();

			/// @brief Handles a client connection to the server
			/// @param clientSocket - in - socket descriptor for the client
			/// @param clientIP - in - address of the client
			void HandleClient(int32_t clientSocket, const std::string& clientIP);

			/// @brief Sends server shutdown message to a client
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
			std::vector<Client> mClients;	

#ifdef WIN32
			WSADATA mWsaData;
#endif
			SOCKET mSocket;
		};
	} // Communications

} // Essentials

#endif // TCP_SERVER
