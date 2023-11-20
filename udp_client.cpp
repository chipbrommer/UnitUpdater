///////////////////////////////////////////////////////////////////////////////
//! @file		udp_client.cpp
//! @brief		Implementation of the udp client class
//! @author		Chip Brommer
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
//  Includes:
//          name                        reason included
//          --------------------        ---------------------------------------
#include	"udp_client.h"				// UDP Client Class
//
///////////////////////////////////////////////////////////////////////////////

namespace Essentials
{
	namespace Communications
	{
		UDP_Client::UDP_Client()
		{
			mTitle				= "UDP Client";
			mLastError			= UdpClientError::NONE;
			mDestinationAddr	= {};
			mClientAddr			= {};
			mBroadcastAddr		= {};
			mLastReceiveInfo	= new Endpoint();
			mTimeout.tv_sec		= UDP_DEFAULT_SOCKET_TIMEOUT;
#if WIN32
			mTimeout.tv_usec	= UDP_DEFAULT_SOCKET_TIMEOUT * 1000;
#else
			mTimeout.tv_usec	= static_cast<__suseconds_t>(UDP_DEFAULT_SOCKET_TIMEOUT) * 1000;
#endif
			mTimeToLive			= 2;

#ifdef WIN32
			if (WSAStartup(MAKEWORD(2, 2), &mWsaData) != 0) 
			{
				mLastError = UdpClientError::WINSOCK_FAILURE;
			}
#endif
			mSocket				= INVALID_SOCKET;
			mBroadcastSocket	= INVALID_SOCKET;
		}

		UDP_Client::UDP_Client(const std::string& clientsAddress, const int16_t clientsPort) : UDP_Client()
		{
			if (ValidateIP(clientsAddress) == -1)
			{
				mLastError = UdpClientError::BAD_ADDRESS;
			}

			if (ValidatePort(clientsPort) == false)
			{
				mLastError = UdpClientError::BAD_PORT;
			}

			// Setup mClientAddr
			memset(reinterpret_cast<char*>(&mClientAddr), 0, sizeof(mClientAddr));
			mClientAddr.sin_family = AF_INET;
			mClientAddr.sin_port = htons(clientsPort);
			if (inet_pton(AF_INET, clientsAddress.c_str(), &(mClientAddr.sin_addr)) <= 0)
			{
				mLastError = UdpClientError::ADDRESS_NOT_SUPPORTED;
			}
		}

		UDP_Client::~UDP_Client()
		{
			DisableBroadcastSender();
			CloseBroadcastListeners();
			CloseMulticastGroups();
			CloseUnicast();

#ifdef WIN32
			WSACleanup();
#endif
		}

		int8_t UDP_Client::ConfigureThisClient(const std::string& address, const int16_t port)
		{
			// Validate port number
			if (ValidatePort(port) == false)
			{
				mLastError = UdpClientError::BAD_PORT;
				return -1;
			}

			// Setup mClientAddr
			memset(reinterpret_cast<char*>(&mClientAddr), 0, sizeof(mClientAddr));

			// if user passed in an address, verify its not empty and attempt to set it to the client. 
			// else it will use IPADDR_ANY
			if (!address.empty())
			{
				if (ValidateIP(address) == -1)
				{
					mLastError = UdpClientError::BAD_ADDRESS;
					return -1;
				}

				if (inet_pton(AF_INET, address.c_str(), &(mClientAddr.sin_addr)) <= 0)
				{
					mLastError = UdpClientError::CONFIGURATION_FAILED;
					return -1;
				}
			}

			mClientAddr.sin_family = AF_INET;
			mClientAddr.sin_port = htons(port);
			mClientAddr.sin_addr.s_addr = INADDR_ANY;

			return 0;
		}

		int8_t  UDP_Client::SetUnicastDestination(const std::string& address, const int16_t port)
		{
			if (ValidateIP(address) == -1)
			{
				mLastError = UdpClientError::BAD_ADDRESS;
				return -1;
			}

			if (ValidatePort(port) == false)
			{
				mLastError = UdpClientError::BAD_PORT;
				return -1;
			}

			// Setup mDestinationAddr
			memset(reinterpret_cast<char*>(&mDestinationAddr), 0, sizeof(mDestinationAddr));
			mDestinationAddr.sin_family = AF_INET;
			mDestinationAddr.sin_port = htons(port);
			if (inet_pton(AF_INET, address.c_str(), &(mDestinationAddr.sin_addr)) <= 0)
			{
				mLastError = UdpClientError::SET_DESTINATION_FAILED;
				return -1;
			}

			return 0;
		}

		int8_t UDP_Client::EnableBroadcastSender(const int16_t port)
		{
			if(mBroadcastSocket != INVALID_SOCKET)
			{
				mLastError = UdpClientError::BROADCAST_ALREADY_ENABLED;
				return -1;
			}

			if (!ValidatePort(port))
			{
				mLastError = UdpClientError::BAD_PORT;
				return -1;
			}

			// Open the broadcast socket
			mBroadcastSocket = socket(AF_INET, SOCK_DGRAM, 0);

			if (mBroadcastSocket == -1)
			{
				mLastError = UdpClientError::BROADCAST_SOCKET_OPEN_FAILURE;
				return -1;
			}

			// Setup broadcast addr
			memset(reinterpret_cast<char*>(&mBroadcastAddr), 0, sizeof(mBroadcastAddr));
			mBroadcastAddr.sin_family = AF_INET;
			mBroadcastAddr.sin_port = htons(port);
			mBroadcastAddr.sin_addr.s_addr = INADDR_BROADCAST;

			// set broadcast option
			int8_t broadcast = 1;
			if (setsockopt(mBroadcastSocket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast)) < 0)
			{
				mLastError = UdpClientError::ENABLE_BROADCAST_FAILED;
				return -1;
			}

			// success
			return 0;
		}

		int8_t UDP_Client::AddBroadcastListener(const int16_t port)
		{
			SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);

			if (sock == INVALID_SOCKET)
			{
				mLastError = UdpClientError::BROADCAST_SOCKET_OPEN_FAILURE;
				return -1;
			}

			// Set the receive timeout
			if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&mTimeout), sizeof(mTimeout)) == SOCKET_ERROR)
			{
				mLastError = UdpClientError::FAILED_TO_SET_TIMEOUT;
				return -1;
			}

			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			addr.sin_addr.s_addr = INADDR_ANY;

			if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
			{
				mLastError = UdpClientError::BIND_FAILED;
				return -1;
			}

			Endpoint ep{ "", port };

			mBroadcastListeners.push_back({sock,addr,ep});

			return 0;
		}

		int8_t UDP_Client::DisableBroadcastSender()
		{
			if (mBroadcastSocket == INVALID_SOCKET)
			{
				mLastError = UdpClientError::BROADCAST_NOT_ENABLED;
				return -1;
			}

			// reset the sockaddr and close the broadcast socket. 
			memset(reinterpret_cast<char*>(&mBroadcastAddr), 0, sizeof(mBroadcastAddr));
			closesocket(mBroadcastSocket);
			mBroadcastSocket = INVALID_SOCKET;

			return 0;
		}

		int8_t UDP_Client::EnableMulticast(const std::string& groupIP, const int16_t groupPort)
		{
			return AddMulticastGroup(groupIP, groupPort);
		}

		int8_t UDP_Client::DisableMulticast()
		{
			if (mMulticastSockets.size() < 1)
			{
				mLastError = UdpClientError::MULTICAST_NOT_ENABLED;
				return -1;
			}

			// success
			CloseMulticastGroups();
			return 0;
		}

		int8_t UDP_Client::AddMulticastGroup(const std::string& groupIP, const int16_t groupPort)
		{
			if (ValidateIP(groupIP) == -1)
			{
				mLastError = UdpClientError::BAD_ADDRESS;
				return -1;
			}

			if (ValidatePort(groupPort) == false)
			{
				mLastError = UdpClientError::BAD_PORT;
				return -1;
			}

			Endpoint ep = { groupIP, groupPort };

			// Create a UDP socket
			SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);

			if (sock == INVALID_SOCKET)
			{
				mLastError = UdpClientError::BAD_MULTICAST_ADDRESS;
				return 1;
			}

			// Enable SO_REUSEADDR to allow multiple sockets to bind to the same address
			int8_t reuseAddr = 1;
			if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseAddr, sizeof(reuseAddr)) == SOCKET_ERROR)
			{
				closesocket(sock);
				mLastError = UdpClientError::ENABLE_REUSEADDR_FAILED;
				return -1;
			}

			// Set up the multicast group to send and receive data from
			sockaddr_in multicastAddr{};
			multicastAddr.sin_family = AF_INET;
			multicastAddr.sin_port = htons(groupPort);
			if (inet_pton(AF_INET, groupIP.c_str(), &(multicastAddr.sin_addr)) <= 0)
			{
				mLastError = UdpClientError::BAD_MULTICAST_ADDRESS;
				return -1;
			}

			// Bind the socket to a local IP address
			sockaddr_in localAddr{};
			localAddr.sin_family = AF_INET;
			localAddr.sin_port = htons(groupPort);
			localAddr.sin_addr.s_addr = INADDR_ANY;

			// Bind the socket to the multicast address
			if (bind(sock, (sockaddr*)&localAddr, sizeof(localAddr)) < 0)
			{
				mLastError = UdpClientError::MULTICAST_BIND_FAILED;
				return 1;
			}

			// Set the TTL (time to live) for any outpoing multicast packets to 5 hops
			if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (const char*)&mTimeToLive, sizeof(mTimeToLive)) == SOCKET_ERROR)
			{
				mLastError = UdpClientError::MULTICAST_SET_TTL_FAILED;
				return -1;
			}

			// Set the outgoing interface for multicast packets
			in_addr interfaceAddr {};
			interfaceAddr.s_addr = INADDR_ANY;
			if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (char*)&interfaceAddr, sizeof(interfaceAddr)) < 0) 
			{
				mLastError = UdpClientError::MULTICAST_INTERFACE_ERROR;
				return -1;
			}

			// Join the multicast group
			ip_mreq multicastRequest{};
			if (inet_pton(AF_INET, groupIP.c_str(), &(multicastRequest.imr_multiaddr)) <= 0)
			{
				mLastError = UdpClientError::BAD_MULTICAST_ADDRESS;
				return -1;
			}

			multicastRequest.imr_interface.s_addr = INADDR_ANY;
			if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&multicastRequest, sizeof(multicastRequest)) == SOCKET_ERROR)
			{
				mLastError = UdpClientError::ADD_MULTICAST_GROUP_FAILED;
				closesocket(sock);
				return -1;
			}

			// Set the socket to non-blocking mode
#ifdef WIN32
			u_long mode = 1;
			if (ioctlsocket(sock, FIONBIO, &mode) != 0) 
			{
				closesocket(sock);
				return -1;
			}
#else
			int flags = fcntl(sock, F_GETFL, 0);
			if (flags == -1)
			{
				mLastError = UdpClientError::FAILED_TO_GET_SOCKET_FLAGS;
				return -1;
			}

			if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1)
			{
				mLastError = UdpClientError::FAILED_TO_SET_NONBLOCK;
				return -1;
			}
#endif

			mMulticastSockets.push_back({ sock, multicastAddr, ep });

			return 0;
		}

		int8_t UDP_Client::OpenUnicast()
		{
			if (mSocket != -1)
			{
				mLastError = UdpClientError::CLIENT_ALREADY_CONNECTED;
				return -1;
			}

			mSocket = socket(AF_INET, SOCK_DGRAM, 0);

			if (mSocket == INVALID_SOCKET)
			{
				mLastError = UdpClientError::SOCKET_OPEN_FAILURE;
				return -1;
			}

			// Set as nonblocking socket
#ifdef WIN32
			u_long nonBlockingMode = 1;
			if (ioctlsocket(mSocket, FIONBIO, &nonBlockingMode) != 0)
			{
				mLastError = UdpClientError::FAILED_TO_SET_NONBLOCK;
				return -1;
			}
#else
			int flags = fcntl(mSocket, F_GETFL, 0);
			if (flags == -1)
			{
				mLastError = UdpClientError::FAILED_TO_GET_SOCKET_FLAGS;
				return -1;
			}

			if (fcntl(mSocket, F_SETFL, flags | O_NONBLOCK) == -1)
			{
				mLastError = UdpClientError::FAILED_TO_SET_NONBLOCK;
				return -1;
			}
#endif
			// Set reuseable address. 
			int8_t opt = 1;
			if (setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0)
			{
				mLastError = UdpClientError::ENABLE_REUSEADDR_FAILED;
				return -1;
			}

			if (bind(mSocket,(sockaddr*)&mClientAddr, sizeof(mClientAddr)) < 0)
			{
				mLastError = UdpClientError::BIND_FAILED;
				return -1;
			}

			return 0;
		}

		int8_t UDP_Client::Send(const char* buffer, const uint32_t size, const SendType type)
		{
			switch (type)
			{
			case SendType::UNICAST:		return SendUnicast(buffer, size);	break;
			case SendType::BROADCAST:	return SendBroadcast(buffer, size); break;
			case SendType::MULTICAST:	return SendMulticast(buffer, size); break;
			default: return -1;
			}

			// default return
			return -1;
		}

		int8_t UDP_Client::SendUnicast(const char* buffer, const uint32_t size)
		{
			// verify socket and then send datagram
			if (mSocket != INVALID_SOCKET)
			{
				int32_t numSent = sendto(mSocket, buffer, size, 0, (sockaddr*)&mDestinationAddr, sizeof(mDestinationAddr));

				if (numSent == -1)
				{
					mLastError = UdpClientError::SEND_FAILED;
					return -1;
				}

				// return success
				return numSent;
			}

			// default return
			return -1;
		}

		int8_t UDP_Client::SendUnicast(const char* buffer, const uint32_t size, const std::string& ipAddress, const int16_t port)
		{
			// verify socket and then send datagram
			if (mSocket != INVALID_SOCKET)
			{
				if (ValidateIP(ipAddress) == -1)
				{
					mLastError = UdpClientError::BAD_ADDRESS;
					return -1;
				}

				if (ValidatePort(port) == false)
				{
					mLastError = UdpClientError::BAD_PORT;
					return -1;
				}

				// Setup mDestinationAddr
				sockaddr_in sentTo{};
				memset(reinterpret_cast<char*>(&sentTo), 0, sizeof(sentTo));
				sentTo.sin_family = AF_INET;
				sentTo.sin_port = htons(port);
				if (inet_pton(AF_INET, ipAddress.c_str(), &(sentTo.sin_addr)) <= 0)
				{
					mLastError = UdpClientError::SET_DESTINATION_FAILED;
					return -1;
				}

				int32_t numSent = 0;
				numSent = sendto(mSocket, buffer, size, 0, (sockaddr*)&sentTo, sizeof(sentTo));

				if (numSent == -1)
				{
					mLastError = UdpClientError::SEND_FAILED;
					return -1;
				}

				// return success
				return numSent;
			}

			// default return
			return -1;
		}

		int8_t UDP_Client::SendBroadcast(const char* buffer, const uint32_t size)
		{
			// verify socket and then send datagram
			if (mBroadcastSocket != INVALID_SOCKET)
			{
				int32_t numSent = sendto(mBroadcastSocket, buffer, size, 0, (sockaddr*)&mBroadcastAddr, sizeof(sockaddr_in));

				if (numSent == -1)
				{
					mLastError = UdpClientError::SEND_BROADCAST_FAILED;
					return -1;
				}

				// return success
				return numSent;
			}

			// default return
			return -1;
		}

		int8_t UDP_Client::SendMulticast(const char* buffer, const uint32_t size, const std::string& groupIP)
		{
			// verify socket and then send datagram
			if (mMulticastSockets.size() > 0)
			{
				int32_t numSent = 0;
				for (const auto& i : mMulticastSockets)
				{
					// Grab the socket and addr info from the vector for use.
					SOCKET sock = std::get<0>(i);
					sockaddr_in addr = std::get<1>(i);
					Endpoint ep = std::get<2>(i);

					// If groupIP is not empty, check the IP we are currently sending to and only send to the desired group.
					if (!groupIP.empty() && groupIP != ep.ipAddress)
					{
						continue;
					}

					numSent = sendto(sock, buffer, size, 0, (sockaddr*)&addr, sizeof(sockaddr_in));

					if (numSent < 0)
					{
						mLastError = UdpClientError::SEND_MULTICAST_FAILED;
						return false;
					}
				}

				return numSent;
			}

			return -1;
		}

		int8_t UDP_Client::ReceiveUnicast(void* buffer, const uint32_t maxSize)
		{
			// Store the data source info
			sockaddr_in sourceAddress{};
			int addressLength = sizeof(sourceAddress);

			// Receive datagram over UDP
#if defined WIN32
			int32_t sizeRead = recvfrom(mSocket, reinterpret_cast<char*>(buffer), maxSize-1, 0, (sockaddr*)&sourceAddress, &addressLength);
#else
			int32_t sizeRead = recvfrom(mSocket, buffer, static_cast<size_t>(maxSize) - 1, 0, (sockaddr*)&sourceAddress, reinterpret_cast<socklen_t*>(&addressLength));
#endif

			// Check for error
			if (sizeRead == -1)
			{
#ifdef WIN32
				int errorCode = WSAGetLastError();
				if (errorCode != WSAEWOULDBLOCK)
				{
					mLastError = UdpClientError::READ_FAILED;
					return -1;
				}
#else
				if (errno != EWOULDBLOCK)
				{
					mLastError = UdpClientError::READ_FAILED;
					return -1;
				}
#endif
				return 0;
			}

			// if data was received, store the port and ip for history. 
			if (sizeRead > 0)
			{
				mLastReceiveInfo->port = ntohs(sourceAddress.sin_port);

				char addr[INET_ADDRSTRLEN];
				if (inet_ntop(AF_INET, &(sourceAddress.sin_addr), addr, INET_ADDRSTRLEN) != NULL)
				{
					mLastReceiveInfo->ipAddress = addr;
				}
			}

			// return size read
			return sizeRead;
		}

		int8_t UDP_Client::ReceiveUnicast(void* buffer, const uint32_t maxSize, std::string& recvFromAddr, int16_t& recvFromPort)
		{
			int8_t rtn = ReceiveUnicast(buffer, maxSize);

			if (rtn > 0)
			{
				recvFromAddr = GetIpOfLastReceive();
				recvFromPort = GetPortOfLastReceive();
			}

			return rtn;;
		}

		int8_t UDP_Client::ReceiveBroadcast(void* buffer, const uint32_t maxSize)
		{
			if (mBroadcastListeners.size() > 0)
			{
				for (const auto& i : mBroadcastListeners)
				{
					// Grab the socket and addr info from the vector for use.
					SOCKET sock = std::get<0>(i);
					sockaddr_in addr = std::get<1>(i);

					if (sock != INVALID_SOCKET)
					{
						// Verify incoming data is available.
						fd_set readSet{};
						FD_ZERO(&readSet);
						FD_SET(sock, &readSet);
						sockaddr_in recvFrom{};
						int recvFromSize = sizeof(recvFrom);
						
						int selectResult = select((int)sock + 1, &readSet, nullptr, nullptr, &mTimeout);
						
						// Catch error
						if (selectResult == SOCKET_ERROR)
						{
							mLastError = UdpClientError::SELECT_READ_ERROR;
							return 1;
						}

						// If data is available on this socket, attempt to read it 
						if (selectResult > 0)
						{
#if defined WIN32
							int32_t receivedBytes = recvfrom(sock, (char*)buffer, maxSize-1, 0, reinterpret_cast<sockaddr*>(&recvFrom), &recvFromSize);
#else
							int32_t receivedBytes = recvfrom(mSocket, buffer, static_cast<size_t>(maxSize) - 1, 0, (sockaddr*)&recvFrom, reinterpret_cast<socklen_t*>(&recvFromSize));
#endif

							if (receivedBytes == SOCKET_ERROR)
							{
#ifdef WIN32
								int errorCode = WSAGetLastError();
								if (errorCode != WSAEWOULDBLOCK)
								{
									mLastError = UdpClientError::RECEIVE_BROADCAST_FAILED;
									return -1;
								}
#else
								if (errno != EWOULDBLOCK)
								{
									mLastError = UdpClientError::RECEIVE_BROADCAST_FAILED;
									return -1;
								}
#endif
								return 0;
							}

							char ip[INET_ADDRSTRLEN];
							inet_ntop(AF_INET, &(recvFrom.sin_addr), ip, INET_ADDRSTRLEN);
							mLastReceiveInfo->ipAddress = std::string(ip);
							mLastReceiveInfo->port = ntohs(addr.sin_port);
							mLastRecvBroadcastPort = mLastReceiveInfo->port;
							return receivedBytes;
						}

						// If here, selectResult == 0, so we check next socket. 
					}
				}

				// If here, for loop completed, and all have no data. 
				return 0;
			}
			return -1;
		}

		int8_t UDP_Client::ReceiveBroadcast(void* buffer, const uint32_t maxSize, int16_t& port)
		{
			int rtn = ReceiveBroadcast(buffer, maxSize);

			if (rtn >= 0)
			{
				port = mLastRecvBroadcastPort;
			}
			return rtn;
		}

		int8_t UDP_Client::ReceiveBroadcastFromListenerPort(void* buffer, const uint32_t maxSize, const int16_t port)
		{
			if (mBroadcastListeners.size() > 0)
			{
				for (const auto& i : mBroadcastListeners)
				{
					// Grab the socket and addr info from the vector for use.
					SOCKET sock = std::get<0>(i);
					sockaddr_in addr = std::get<1>(i);
					Endpoint ep = std::get<2>(i);

					// If the ports arent equal, continue to next iteration.
					if (port != ep.port)
					{
						continue;
					}

					if (sock != INVALID_SOCKET)
					{
						// Verify incoming data is available.
						fd_set readSet{};
						FD_ZERO(&readSet);
						FD_SET(sock, &readSet);
						sockaddr_in recvFrom{};
						int recvFromSize = sizeof(recvFrom);

						int selectResult = select((int)sock + 1, &readSet, nullptr, nullptr, &mTimeout);

						// Catch error
						if (selectResult == SOCKET_ERROR)
						{
							mLastError = UdpClientError::SELECT_READ_ERROR;
							return 1;
						}

						// If data is available on this socket, attempt to read it 
						if (selectResult > 0)
						{
#if defined WIN32
							int32_t receivedBytes = recvfrom(sock, (char*)buffer, maxSize - 1, 0, reinterpret_cast<sockaddr*>(&recvFrom), &recvFromSize);
#else
							int32_t receivedBytes = recvfrom(mSocket, buffer, static_cast<size_t>(maxSize) - 1, 0, (sockaddr*)&recvFrom, reinterpret_cast<socklen_t*>(&recvFromSize));
#endif

							if (receivedBytes == SOCKET_ERROR)
							{
#ifdef WIN32
								int errorCode = WSAGetLastError();
								if (errorCode != WSAEWOULDBLOCK)
								{
									mLastError = UdpClientError::RECEIVE_BROADCAST_FAILED;
									return -1;
								}
#else
								if (errno != EWOULDBLOCK)
								{
									mLastError = UdpClientError::RECEIVE_BROADCAST_FAILED;
									return -1;
								}
#endif
								return 0;
							}

							char ip[INET_ADDRSTRLEN];
							inet_ntop(AF_INET, &(recvFrom.sin_addr), ip, INET_ADDRSTRLEN);
							mLastReceiveInfo->ipAddress = std::string(ip);
							mLastReceiveInfo->port = ntohs(addr.sin_port);
							mLastRecvBroadcastPort = mLastReceiveInfo->port;
							return receivedBytes;
						}

						// If here, selectResult == 0, so we check next socket. 
					}
				}

				// If here, for loop completed, and all have no data. 
				return 0;
			}
			return -1;
		}

		int8_t UDP_Client::ReceiveMulticast(void* buffer, const uint32_t maxSize, std::string& multicastGroup)
		{
			if (mMulticastSockets.size() > 0)
			{
				for (const auto& i : mMulticastSockets)
				{
					// Grab the socket and addr info from the vector for use.
					SOCKET sock = std::get<0>(i);
					sockaddr_in addr = std::get<1>(i);

					if (sock != INVALID_SOCKET)
					{
						// Verify incoming data is available.
						fd_set readSet{};
						FD_ZERO(&readSet);
						FD_SET(sock, &readSet);
						sockaddr_in recvFrom{};
						int recvFromSize = sizeof(recvFrom);

						int selectResult = select(0, &readSet, nullptr, nullptr, &mTimeout);

						// Catch error
						if (selectResult == SOCKET_ERROR)
						{
							mLastError = UdpClientError::SELECT_READ_ERROR;
							return 1;
						}

						// If data is available on this socket, attempt to read it 
						if (selectResult > 0)
						{
#if defined WIN32
							int32_t receivedBytes = recvfrom(sock, (char*)buffer, maxSize - 1, 0, reinterpret_cast<sockaddr*>(&recvFrom), &recvFromSize);
#else
							int32_t receivedBytes = recvfrom(mSocket, buffer, static_cast<size_t>(maxSize) - 1, 0, (sockaddr*)&recvFrom, reinterpret_cast<socklen_t*>(&recvFromSize));
#endif

							if (receivedBytes == SOCKET_ERROR)
							{
#ifdef WIN32
								int errorCode = WSAGetLastError();
								if (errorCode != WSAEWOULDBLOCK)
								{
									mLastError = UdpClientError::RECEIVE_BROADCAST_FAILED;
									return -1;
								}
#else
								if (errno != EWOULDBLOCK)
								{
									mLastError = UdpClientError::RECEIVE_BROADCAST_FAILED;
									return -1;
								}
#endif
								return 0;
							}

							char address[INET_ADDRSTRLEN];
							if (inet_ntop(AF_INET, &(addr.sin_addr), address, INET_ADDRSTRLEN) != NULL)
							{
								multicastGroup = address;
							}

							return receivedBytes;
						}

						// If here, selectResult == 0, so we check next socket. 
					}
				}

				// If here, for loop completed, and all have no data. 
				return 0;
			}
			return -1;
		}

		void UDP_Client::CloseUnicast()
		{
			closesocket(mSocket);
			mSocket = INVALID_SOCKET;
		}

		void UDP_Client::CloseBroadcastListeners()
		{
			closesocket(mBroadcastSocket);
			mBroadcastSocket = INVALID_SOCKET;

			for (const auto& i : mBroadcastListeners)
			{
				closesocket(std::get<0>(i));
			}
			
			mBroadcastListeners.clear();
		}

		void UDP_Client::CloseMulticastGroups()
		{
			for (const auto& i : mMulticastSockets)
			{
				closesocket(std::get<0>(i));
			}

			mMulticastSockets.clear();
		}

		int8_t UDP_Client::SetTimeToLive(const int8_t ttl)
		{
			if (ttl > 0 && ttl < 255)
			{
				mTimeToLive = ttl;

				if (mMulticastSockets.size() > 0)
				{
					for (const auto& i : mMulticastSockets)
					{
						SOCKET sock = std::get<0>(i);

						if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (const char*)&mTimeToLive, sizeof(mTimeToLive)) == SOCKET_ERROR)
						{
							mLastError = UdpClientError::MULTICAST_SET_TTL_FAILED;
							return -1;
						}
					}
				}

				return 0;
			}

			return -1;
		}

		int8_t UDP_Client::SetTimeout(const int32_t timeoutMSecs)
		{
			mTimeout.tv_sec = timeoutMSecs / 1000;
#if WIN32
			mTimeout.tv_usec = timeoutMSecs * 1000;
#else
			mTimeout.tv_usec = static_cast<__suseconds_t>(timeoutMSecs) * 1000;
#endif
			return 0;
		}

		std::string UDP_Client::GetIpOfLastReceive()
		{
			return mLastReceiveInfo->ipAddress;
		}

		int16_t UDP_Client::GetPortOfLastReceive()
		{
			return mLastReceiveInfo->port;
		}

		void UDP_Client::GetLastSendersInfo(std::string& ip, int& port)
		{
			ip = mLastReceiveInfo->ipAddress;
			port = mLastReceiveInfo->port;
		}

		std::string UDP_Client::GetLastError()
		{
			return UdpClientErrorMap[mLastError];
		}
	
		int8_t UDP_Client::ValidateIP(const std::string& ip)
		{
			sockaddr_in sa4 = {};
			sockaddr_in6 sa6 = {};

			// Check if it's a valid IPv4 address
			if (inet_pton(AF_INET, ip.c_str(), &(sa4.sin_addr)) == 1)
			{
				return 1;
			}

			// Check if it's a valid IPv6 address
			if (inet_pton(AF_INET6, ip.c_str(), &(sa6.sin6_addr)) == 1)
			{
				return 2;
			}

			return -1;  // Invalid IP address
		}

		bool UDP_Client::ValidatePort(const int16_t port)
		{
			return (port >= 0 && port <= 65535);
		}

	}
}