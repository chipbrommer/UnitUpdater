///////////////////////////////////////////////////////////////////////////////
//! @file		udp_client.h
//! @brief		A cross platform class to handle UDP communication.
//! @author		Chip Brommer
///////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////
//
//  Includes:
//          name                        reason included
//          --------------------        ---------------------------------------
#ifdef WIN32
#include <stdint.h>						// Standard integer types
#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
const int INVALID_SOCKET = -1;
const int SOCKET_ERROR = -1;
const int SD_BOTH = SHUT_RDWR;
#define closesocket(s) close(s)
#endif
#include <map>							// Error enum to strings.
#include <string>						// Strings
#include <regex>						// Regular expression for ip validation
//
//	Defines:
//          name                        reason defined
//          --------------------        ---------------------------------------
#ifndef     CPP_UDP_CLIENT				// Define the cpp UDP client class. 
#define     CPP_UDP_CLIENT
//
///////////////////////////////////////////////////////////////////////////////

namespace Essentials
{
	namespace Communications
	{
		constexpr static uint8_t	UDP_CLIENT_VERSION_MAJOR	= 0;
		constexpr static uint8_t	UDP_CLIENT_VERSION_MINOR	= 1;
		constexpr static uint8_t	UDP_CLIENT_VERSION_PATCH	= 0;
		constexpr static uint8_t	UDP_CLIENT_VERSION_BUILD	= 0;
		constexpr static uint8_t	UDP_DEFAULT_SOCKET_TIMEOUT	= 1;

		static std::string UdpClientVersion = "UDP Client v" +
			std::to_string((uint8_t)UDP_CLIENT_VERSION_MAJOR) + "." +
			std::to_string((uint8_t)UDP_CLIENT_VERSION_MINOR) + "." +
			std::to_string((uint8_t)UDP_CLIENT_VERSION_PATCH) + " - b" +
			std::to_string((uint8_t)UDP_CLIENT_VERSION_BUILD) + ".\n";

		/// <summary>enum for error codes</summary>
		enum class UdpClientError : uint8_t
		{
			NONE,
			BAD_ADDRESS,
			ADDRESS_NOT_SET,
			BAD_PORT,
			PORT_NOT_SET,
			CLIENT_ALREADY_CONNECTED,
			FAILED_TO_CONNECT,
			WINSOCK_FAILURE,
			SOCKET_OPEN_FAILURE,
			ADDRESS_NOT_SUPPORTED,
			CONNECTION_FAILED,
			SEND_FAILED,
			READ_FAILED,
			ENABLE_MULTICAST_FAILED,
			DISABLE_MULTICAST_FAILED,
			ENABLE_BROADCAST_FAILED,
			DISABLE_BROADCAST_FAILED,
			SEND_MULTICAST_FAILED,
			SEND_BROADCAST_FAILED,
			CONFIGURATION_FAILED,
			SET_DESTINATION_FAILED,
			BIND_FAILED,
			BROADCAST_ALREADY_ENABLED,
			BROADCAST_SOCKET_OPEN_FAILURE,
			BROADCAST_NOT_ENABLED,
			MULTICAST_SOCKET_FAILED,
			BAD_MULTICAST_ADDRESS,
			FAILED_TO_SET_NONBLOCK,
			FAILED_TO_GET_SOCKET_FLAGS,
			ENABLE_REUSEADDR_FAILED,
			FAILED_TO_SET_TIMEOUT,
			SELECT_READ_ERROR,
			RECEIVE_BROADCAST_FAILED,
			MULTICAST_NOT_ENABLED,
			ADD_MULTICAST_GROUP_FAILED,
			MULTICAST_INTERFACE_ERROR,
			MULTICAST_BIND_FAILED,
			MULTICAST_SET_TTL_FAILED,
		};

		/// <summary>Error enum to string map</summary>
		static std::map<UdpClientError, std::string> UdpClientErrorMap
		{
			{UdpClientError::NONE,
			std::string("Error Code " + std::to_string((uint8_t)UdpClientError::NONE) + ": No error.")},
			{UdpClientError::BAD_ADDRESS,
			std::string("Error Code " + std::to_string((uint8_t)UdpClientError::BAD_ADDRESS) + ": Bad address.")},
			{UdpClientError::ADDRESS_NOT_SET,
			std::string("Error Code " + std::to_string((uint8_t)UdpClientError::ADDRESS_NOT_SET) + ": Address not set.")},
			{UdpClientError::BAD_PORT,
			std::string("Error Code " + std::to_string((uint8_t)UdpClientError::BAD_PORT) + ": Bad port.")},
			{UdpClientError::PORT_NOT_SET,
			std::string("Error Code " + std::to_string((uint8_t)UdpClientError::PORT_NOT_SET) + ": Port not set.")},
			{UdpClientError::FAILED_TO_CONNECT,
			std::string("Error Code " + std::to_string((uint8_t)UdpClientError::FAILED_TO_CONNECT) + ": Failed to connect.")},
			{UdpClientError::WINSOCK_FAILURE,
			std::string("Error Code " + std::to_string((uint8_t)UdpClientError::WINSOCK_FAILURE) + ": Winsock creation failure.")},
			{UdpClientError::SOCKET_OPEN_FAILURE,
			std::string("Error Code " + std::to_string((uint8_t)UdpClientError::SOCKET_OPEN_FAILURE) + ": Socket open failure.")},
			{UdpClientError::ADDRESS_NOT_SUPPORTED,
			std::string("Error Code " + std::to_string((uint8_t)UdpClientError::ADDRESS_NOT_SUPPORTED) + ": Address not supported.")},
			{UdpClientError::CONNECTION_FAILED,
			std::string("Error Code " + std::to_string((uint8_t)UdpClientError::CONNECTION_FAILED) + ": Connection failed.")},
			{UdpClientError::SEND_FAILED,
			std::string("Error Code " + std::to_string((uint8_t)UdpClientError::SEND_FAILED) + ": Send failed.")},
			{UdpClientError::READ_FAILED,
			std::string("Error Code " + std::to_string((uint8_t)UdpClientError::READ_FAILED) + ": Read failed.")},
		};

		/// <summary>Represents an endpoint for a connection</summary>
		struct Endpoint
		{
			std::string ipAddress = "";
			int16_t	port = 0;
		};

		/// <summary>Send Type for the Send Function.</summary>
		enum class SendType : uint8_t
		{
			UNICAST,
			BROADCAST,
			MULTICAST,
		};

		/// <summary>A multi-platform class to handle UDP communications.</summary>
		class UDP_Client
		{
		public:
			/// <summary>Default Constructor</summary>
			UDP_Client();

			/// <summary>Constructor to receive an address and port</summary>
			UDP_Client(const std::string& clientsAddress, const int16_t clientsPort);

			/// <summary>Default Deconstructor</summary>
			~UDP_Client();

			/// <summary>Configure the address and port of this client</summary>
			/// <param name="address"> -[in]- Address of this client</param>
			/// <param name="port"> -[in]- Address of this client</param>
			/// <returns>0 if successful, -1 if fails. Call Serial::GetLastError to find out more.</returns>
			int8_t ConfigureThisClient(const std::string& address, const int16_t port);

			/// <summary>Set this unicast destination</summary>
			/// <param name="address"> -[in]- Address to sent to</param>
			/// <param name="port"> -[in]- Port to sent to</param>
			/// <returns>0 if successful, -1 if fails. Call Serial::GetLastError to find out more.</returns>
			int8_t SetUnicastDestination(const std::string& address, const int16_t port);

			/// <summary>A function to enable broadcasting</summary>
			/// <param name="port"> -[in]- Port to broadcast on</param>
			//// <returns>0 if successful, -1 if fails. Call Serial::GetLastError to find out more.</returns>
			int8_t EnableBroadcastSender(const int16_t port);

			/// <summary>A function to add a port to listen for broadcast messages</summary>
			/// <param name="port"> -[in]- Port to listen for broadcast on</param>
			//// <returns>0 if successful, -1 if fails. Call Serial::GetLastError to find out more.</returns>
			int8_t AddBroadcastListener(const int16_t port);

			/// <summary>Disables broadcast and cleans up</summary>
			/// <returns>0 if successful, -1 if fails. Call Serial::GetLastError to find out more.</returns>
			int8_t DisableBroadcast();

			/// <summary>Enables multicast sockets and adds the first group. To add more groups, use AddMulticastGroup</summary>
			/// <param name="groupIP"> -[in]- Address of multicast group.</param>
			/// <param name="groupPort"> -[in]- Port of multicast group.</param>
			/// <returns>0 if successful, -1 if fails. Call Serial::GetLastError to find out more.</returns>
			int8_t EnableMulticast(const std::string& groupIP, const int16_t groupPort);

			/// <summary>Disables multicast and cleans up</summary>
			/// <returns>0 if successful, -1 if fails. Call Serial::GetLastError to find out more.</returns>
			int8_t DisableMulticast();

			/// <summary>Add an enpoint to the list of multicast recepients.</summary>
			/// <param name="groupIP"> -[in]- Address of multicast group.</param>
			/// <param name="groupPort"> -[in]- Port of multicast group.</param>
			/// <returns>0 if successful, -1 if fails. Call Serial::GetLastError to find out more.</returns>
			int8_t AddMulticastGroup(const std::string& groupIP, const int16_t port);

			/// <summary>Opens the UDP unicast socket and binds it to the set address and port</summary>
			/// <returns>0 if successful, -1 if fails. Call Serial::GetLastError to find out more.</returns>
			int8_t OpenUnicast();

			/// <summary>Send a message over a specified socket type</summary>
			/// <param name="buffer"> -[in]- Buffer to be sent</param>
			/// <param name="size"> -[in]- Size to be sent</param>
			/// <returns>0+ if successful (number bytes sent), -1 if fails. Call UDP_Client::GetLastError to find out more.</returns>
			int8_t Send(const char* buffer, const uint32_t size, const SendType type);

			/// <summary>Sends a unicast message</summary>
			/// <param name="buffer"> -[in]- Buffer to be sent</param>
			/// <param name="size"> -[in]- Size to be sent</param>
			/// <returns>0+ if successful (number bytes sent), -1 if fails. Call UDP_Client::GetLastError to find out more.</returns>
			int8_t SendUnicast(const char* buffer, const uint32_t size);

			/// <summary>Send a unicast message to specified ip and port</summary>
			/// <param name="buffer"> -[in]- Buffer to be sent</param>
			/// <param name="size"> -[in]- Size to be sent</param>
			/// <returns>0+ if successful (number bytes sent), -1 if fails. Call UDP_Client::GetLastError to find out more.</returns>
			int8_t SendUnicast(const char* buffer, const uint32_t size, const std::string& ipAddress, const int16_t port);

			/// <summary>Send a broadcast message</summary>
			/// <param name="buffer"> -[in]- Buffer to be sent</param>
			/// <param name="size"> -[in]- Size to be sent</param>
			/// <returns>0+ if successful (number bytes sent), -1 if fails. Call UDP_Client::GetLastError to find out more.</returns>
			int8_t SendBroadcast(const char* buffer, const uint32_t size);

			/// <summary>Send a multicast message to all joined groups</summary>
			/// <param name="buffer"> -[in]- Buffer to be sent</param>
			/// <param name="size"> -[in]- Size to be sent</param>
			/// <param name="groupIP"> -[in/opt]- IP of group to send to if only sending to one desired group</param>
			/// <returns>0+ if successful (number bytes sent), -1 if fails. Call UDP_Client::GetLastError to find out more.</returns>
			int8_t SendMulticast(const char* buffer, const uint32_t size, const std::string& groupIP = "");

			/// <summary>Receive data from a server</summary>
			/// <param name="buffer"> -[out]- Buffer to place received data into</param>
			/// <param name="maxSize"> -[in]- Maximum number of bytes to be read</param>
			/// <returns>0+ if successful (number bytes received), -1 if fails. Call UDP_Client::GetLastError to find out more.</returns>
			int8_t ReceiveUnicast(void* buffer, const uint32_t maxSize);

			/// <summary>Receive data from a server and get the IP and Port of the sender</summary>
			/// <param name="buffer"> -[out]- Buffer to place received data into</param>
			/// <param name="maxSize"> -[in]- Maximum number of bytes to be read</param>
			/// <param name="recvFromAddr"> -[out]- IP Address of the sender</param>
			/// <param name="recvFromPort"> -[out]- Port of the sender</param>
			/// <returns>0+ if successful (number bytes received), -1 if fails. Call UDP_Client::GetLastError to find out more.</returns>
			int8_t ReceiveUnicast(void* buffer, const uint32_t maxSize, std::string& recvFromAddr, int16_t& recvFromPort);

			/// <summary>Receive a broadcast message</summary>
			/// <param name="buffer"> -[out]- Buffer to place received data into</param>
			/// <param name="maxSize"> -[in]- Maximum number of bytes to be read</param>
			/// <returns>0+ if successful (number bytes received), -1 if fails. Call UDP_Client::GetLastError to find out more.</returns>
			int8_t ReceiveBroadcast(void* buffer, const uint32_t maxSize);

			/// <summary>Receive a broadcast message</summary>
			/// <param name="buffer"> -[out]- Buffer to place received data into</param>
			/// <param name="maxSize"> -[in]- Maximum number of bytes to be read</param>
			/// <param name="port"> -[out]- Port the broadcast was received from</param>
			/// <returns>0+ if successful (number bytes received), -1 if fails. Call UDP_Client::GetLastError to find out more.</returns>
			int8_t ReceiveBroadcast(void* buffer, const uint32_t maxSize, int16_t& port);

			/// <summary>Receive a broadcast message from a specific listener port</summary>
			/// <param name="buffer"> -[out]- Buffer to place received data into</param>
			/// <param name="maxSize"> -[in]- Maximum number of bytes to be read</param>
			/// <param name="port"> -[in]- Port of the broadcast to receive from</param>
			/// <returns>0+ if successful (number bytes received), -1 if fails. Call UDP_Client::GetLastError to find out more.</returns>
			int8_t ReceiveBroadcastFromListenerPort(void* buffer, const uint32_t maxSize, const int16_t port);

			/// <summary>Receive a multicast message</summary>
			/// <param name="buffer"> -[out]- Buffer to place received data into</param>
			/// <param name="maxSize"> -[in]- Maximum number of bytes to be read</param>
			/// <param name="multicastGroup"> -[out]- IP of the group received from</param>
			/// <returns>0+ if successful (number bytes received), -1 if fails. Call UDP_Client::GetLastError to find out more.</returns>
			int8_t ReceiveMulticast(void* buffer, const uint32_t maxSize, std::string& multicastGroup);

			/// <summary>Closes the unicast client and cleans up</summary>
			void CloseUnicast();

			/// <summary>Closes the broadcast client and cleans up</summary>
			void CloseBroadcast();

			/// <summary>Closes the broadcast client and cleans up</summary>
			void CloseMulticast();

			/// <summary>Sets the number of hops outbound multicast messages live for.</summary>
			/// <param name="ttl"> -[in]- Number of hops multicast messages live for 0-255</param>
			/// <returns>0 if successful set, -1 if fails. Call UDP_Client::GetLastError to find out more..</returns>
			int8_t SetTimeToLive(const int8_t ttl);

			/// <summary>Sets the timeout value for message reads.</summary>
			/// <param name="timeoutMSecs"> -[in]- Number of milliseconds for a read timeout.</param>
			/// <returns>0 if successful set, -1 if fails. Call UDP_Client::GetLastError to find out more.</returns>
			int8_t SetTimeout(const int32_t timeoutMSecs);

			/// <summary>Get the ip address of the last received message.</summary>
			/// <returns>If valid, A string containing the IP address; else an empty string. Call UDP_Client::GetLastError to find out more.</returns>
			std::string GetIpOfLastReceive();

			/// <summary>Get the port number of the last received message.</summary>
			/// <returns>The port number, else -1 on error. Call UDP_Client::GetLastError to find out more.</returns>
			int16_t GetPortOfLastReceive();

			/// <summary>Get the last error in string format</summary>
			/// <returns>The last error in a formatted string</returns>
			std::string GetLastError();

		protected:
		private:
			/// <summary>Validates an IP address is IPv4 or IPv6</summary>
			/// <param name="ip"> -[in]- IP Address to be validated</param>
			/// <returns>1 if valid ipv4, 2 if valid ipv6, else -1 on fail</returns>
			int8_t ValidateIP(const std::string& ip);

			/// <summary>Validates a port number is between 0-65535</summary>
			/// <param name="port"> -[in]- Port number to be validated</param>
			/// <returns>true = valid, false = invalid</returns>
			bool ValidatePort(const int16_t port);

			// Variables
			std::string					mTitle;					// Title for this utility when using CPP_Logger
			UdpClientError				mLastError;				// Last error for this utility
			sockaddr_in					mDestinationAddr;		// Destination sockaddr
			sockaddr_in					mClientAddr;			// This clients sockaddr
			sockaddr_in					mBroadcastAddr;			// Broadcast sockaddr
			Endpoint*					mLastReceiveInfo;		// Last receive endpoint info
			timeval						mTimeout;				// Holds the message receive timeout value in seconds. 
			int8_t						mTimeToLive;			// Holds the ttl (Time To Live) for multicast messages. IE: How many interface hops they live for: 0-255
			int16_t						mLastRecvBroadcastPort;	// Holds port of last received broadcast port

#ifdef WIN32
			WSADATA						mWsaData;				// Winsock data
#endif
			SOCKET						mSocket;				// socket FD for this client
			SOCKET						mBroadcastSocket;		// socket FD for broadcasting
			std::vector<std::tuple<SOCKET, sockaddr_in, Endpoint>>   mBroadcastListeners;	// Vector of tuples containing the socket and addr info for listening to broadcasts
			std::vector<std::tuple<SOCKET, sockaddr_in, Endpoint>>   mMulticastSockets;		// Vector of tuples containing the socket and addr info for multicasts
		};
	}
}

#endif		// CPP_UDP_CLIENT