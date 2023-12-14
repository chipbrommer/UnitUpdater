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
#include <functional>
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
			SOCKET socket;					// socket of the client
			std::int32_t bytesReceived;		// number of bytes received from the client
			std::int32_t bytesWritten;		// number of bytes written to the client
			std::int32_t timeConnected;		// the timestamp when the client was connected to the server
			std::mutex mutex;				// data safety mutex

			// Default constructor
			Client() : ip(""), port(0), socket(-1), bytesReceived(0), bytesWritten(0), timeConnected(0) {}

			// Constructor with parameters
			Client(const std::string& ip, const std::uint16_t port, std::int32_t socket, std::int32_t bytesReceived,
				std::int32_t bytesWritten, std::int32_t timeConnected)
				: ip(ip), port(port), socket(socket), bytesReceived(bytesReceived),
				bytesWritten(bytesWritten), timeConnected(timeConnected) {}

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
				SEND_FAILED,
			};

			/// @brief Error enum to string map
			static std::map<TcpServerError, std::string> TcpServerErrorMap;

			/// @brief Default constructor
			TCP_Server();

			/// @brief Constructor with max clients 
			/// @param maxClients - in - Maximum number of clients the server should handle concurrently
			TCP_Server(const int maxClients);

			/// @brief Constructor with defaults 
			/// @param maxClients - in - Maximum number of clients the server should handle concurrently
			/// @param address - in - Address to serve the server on.
			/// @param port - in - Port to serve the server on.
			explicit TCP_Server(const int maxClients, const std::string& address, const int port);

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

			/// @brief a blocking function that runs the server interface and listens for clients communication
			int Run();

			/// @brief Stops the server if it is running
			void Stop();

			/// @brief Sends a buffer to a client 
			/// @param clientFD - in - the file descriptor for the client to send to
			/// @param msg - in - the buffer to be sent to the client
			/// @param msgSize - in - the size of the data to be sent. 
			/// @return -1 on error, else number of bytes sent
			int SendBufferToClient(const int clientFD, const uint8_t* msg, const int msgSize);

			/// @brief Sends a message to a client 
			/// @param clientFD - in - the file descriptor for the client to send to
			/// @param msg - in - the buffer to be sent to the client
			/// @return -1 on error, else number of bytes sent
			int SendMessageToClient(const int clientFD, const std::string& msg);

			/// @brief Get the last error in string format
			/// @return The last error in a formatted string
			std::string GetLastError();

			/// @brief Set a function to be called when a new connection is established
			/// @param handler - in - Function to be used as a callback for a new connection 
			void SetConnectionCallback(const std::function<int(const int)>& handler);

			/// @brief Set a function to be called when a new message is received from a client
			/// @param handler - in - Function to be used as a callback for a new message 
			void SetMessageCallback(const std::function<int(const int, const std::string&)>& handler);

			/// @brief Set a function to be called when a client disconnects
			/// @param handler - in - Function to be used as a callback for a client disconnect
			void SetDisconnectCallback(const std::function<int(const int)>& handler);

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

			/// @brief Sends server shutdown message to a client
			int SendShutdownMessage(SOCKET clientSocket);

			/// @brief Close a single client connection
			void CloseClientSocket(SOCKET clientSocket);
			
			/// @brief Close all client sockets in mClient vector
			void CloseAllClientSockets();

			std::string mAddress;				// Address of the TCP server
			int mPort;							// Port of the TCP server
			int mMaxClients;					// Holds maximum number of allowed client connections
			TcpServerError mLastError;			// Holds last error of the TCP server
			std::atomic<bool> mStopFlag;		// Stop flag for the server. 
			std::vector<Client> mClients;		// Vector of clients
			SOCKET mSocket;						// Server socket

			// callback function to be called when server gets a new connection
			std::function<int(const int fd)> mNewConnectionHandler;

			// callback function to be called when server receives a message
			std::function<int(const int fd, const std::string& msg)> mMessageHandler;

			// callback function to be called when client disconnects
			std::function<int(const int fd)> mDisconnectHandler;						

#ifdef WIN32
			WSADATA mWsaData;					// Win socket data
			fd_set	mFDs;						// Windows FD list
#else
			pollfd* mFDs;						// Pointer for an array of file descriptors
#endif
		};

	} // Communications
} // Essentials

#endif // TCP_SERVER
