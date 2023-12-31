﻿#include "UnitUpdater.h"

UnitUpdater::UnitUpdater() 
{
	// Initialize
	mLastError				= 0;
	mMaxBroadcastListeningTimeInMSec	= DEFAULT_TIMELENGTH_MSEC;
    mBroadcastPort			= 0;
    mServerPort				= 0;
	mCloseRequested			= false;
	mUpdateInProgress		= false;
	mUdp					= new Essentials::Communications::UDP_Client();
	mTimer					= Essentials::Utilities::Timer::GetInstance();

	// Welcome message
	std::cout << "------------------------------------\n";
	std::cout << "-           Unit Updater           -\n";
	std::cout << "------------------------------------\n" << std::endl;
}

UnitUpdater::UnitUpdater(int bPort, int tPort) : UnitUpdater()
{
	mBroadcastPort = bPort;
	mServerPort = tPort;
}

UnitUpdater::~UnitUpdater()
{
	Close();
}

int UnitUpdater::Setup(std::string filepath, int preferredBroadcastPort, int preferredCommsPort)
{
	// Attempt to load the settings from filepath.
	std::ifstream settingsFile(filepath);
	if (settingsFile.is_open())
	{
		nlohmann::json settingsJson;
		settingsFile >> settingsJson;
		settingsFile.close();
		mSettings.LoadFromJson(settingsJson);

		std::cout << "[UPDATER] Settings Loaded Successfully\n";
		mBroadcastPort = mSettings.broadcastPort;
		mServerPort = mSettings.communicationPort;
		mMaxBroadcastListeningTimeInMSec = mSettings.broadcastTimeoutMSec;
	}
	else
	{
		std::cout << "[UPDATER] Failed to Load Settings\n";
		return -1;
	}

	// Use preferred ports if they're not 0
	if (preferredBroadcastPort != 0) mBroadcastPort = preferredBroadcastPort;
	if (preferredCommsPort != 0) mServerPort = preferredCommsPort;

	// Setup TCP to serve on any network interface
	mTcp = new Essentials::Communications::TCP_Server(mSettings.maximumConnections, "0.0.0.0", mServerPort);

	if (mTcp == nullptr) 
	{
		std::cout << "[UPDATER] Failed to initialize TCP server\n";
		return -1;
	}

	// Create a lambda function to pass HandleMessage as the message callback 
	auto handleMessageCallback = [this](const int clientFd, const std::string& msg) {
		return HandleMessage(clientFd, msg);
	};
	mTcp->SetMessageCallback(handleMessageCallback);

	// Default return
	return 0;
}

void UnitUpdater::SetMaxBroadcastListeningTime(int msecTimeout)
{
	mMaxBroadcastListeningTimeInMSec = msecTimeout;
}

int UnitUpdater::StartServer()
{
	if (mServerPort == 0)
	{
		return -1;
	}

	// Start the tcp connection
	if (mTcp->Start() < 0)
	{
		std::cerr << "[UPDATER] Failed to start the server." << std::endl;
		std::cout << mTcp->GetLastError();
		return -1;
	}

	while(!mCloseRequested)
	{
		// tcp server running
	}

	mTcp->Stop();
	return 0;
}

int UnitUpdater::HandleMessage(const int clientFD, const std::string& data)
{
	if (IsPacketValid(reinterpret_cast<const uint8_t*>(data.c_str())))
	{
		UPDATER_ACTION_MESSAGE msg = { 0 };
		memcpy(&msg, data.c_str(), sizeof(msg));

		switch (msg.action)
		{
		case ACTION_COMMAND::CLOSE:					
			mCloseRequested = true;
			break;
		case ACTION_COMMAND::BOOT_INTERRUPT:
			// Handled in ListenForInterrupt()
			break;
		case ACTION_COMMAND::GET_AS_BUILT:
			{
				std::string response;
				RESPONSE_MSG msgOut;

				// Read the contents of the JSON file
				std::ifstream inputFile(mSettings.asBuiltLocation);

				if (inputFile.is_open()) 
				{
					std::string line;

					while (getline(inputFile, line)) 
					{
						response += line;
					}
					inputFile.close();

					msgOut.action = ACTION_COMMAND::GET_AS_BUILT;
					msgOut.status = ACTION_STATUS::SUCCESS;
					msgOut.data = response;
				}
				else 
				{
					// Error opening file - respond with failure
					msgOut.action = ACTION_COMMAND::GET_AS_BUILT;
					msgOut.status = ACTION_STATUS::FAIL;
					msgOut.data = response;
				}

				std::string serializedData = SerializeResponseMsg(msgOut);
				mTcp->SendBufferToClient(clientFD, reinterpret_cast<const uint8_t*>(serializedData.data()), serializedData.size());
			}
			break;
		case ACTION_COMMAND::UPDATE_OFS:
			mUpdateInProgress = true;
			// @todo - write bytes to a temp file until we have received all the bytes
			break;
		case ACTION_COMMAND::UPDATE_CONFIG:
		{
			// @todo - take the received data and validate it and then write to the file if its good. 

			std::string response;
			RESPONSE_MSG msgOut;

			// Read the contents of the JSON file
			std::ifstream inputFile(mSettings.ofsLocation);

			if (inputFile.is_open())
			{
				std::string line;

				while (getline(inputFile, line))
				{
					response += line;
				}
				inputFile.close();

				msgOut.action = ACTION_COMMAND::UPDATE_CONFIG;
				msgOut.status = ACTION_STATUS::SUCCESS;
				msgOut.data = response;
			}
			else
			{
				// Error opening file - respond with failure
				msgOut.action = ACTION_COMMAND::GET_AS_BUILT;
				msgOut.status = ACTION_STATUS::FAIL;
				msgOut.data = response;
			}

			std::string serializedData = SerializeResponseMsg(msgOut);
			mTcp->SendBufferToClient(clientFD, reinterpret_cast<const uint8_t*>(serializedData.data()), serializedData.size());
		}
		break;
		case ACTION_COMMAND::GET_LOG_NAMES:
			{

			}
			break;
		case ACTION_COMMAND::GET_SPECIFIC_LOG:
			{

			}
			break;
		case ACTION_COMMAND::GET_LAST_FLIGHT_LOG:
			{

			}
			break;
		}
	}

	return -1;
}

int UnitUpdater::ListenForInterrupt()
{
	// Validate we have a broadcast port and attempt to add the listener. 
	if (mBroadcastPort <= 0 ||
		mUdp->AddBroadcastListener(mBroadcastPort) < 0)
	{
		return -1;
	}

	uint8_t buffer[200];					// buffer to hold data received
	int size = sizeof(buffer);				// size of the buffer
	int start = mTimer->GetMSecTicks();		// time of start
	int elapsed = 0;						// time elapsed count
	bool packetReceived = false;			// Verify the packet is what we desire

	// While the time length hasnt elapsed
	while (elapsed <= mMaxBroadcastListeningTimeInMSec)
	{
		int bytesReceived = mUdp->ReceiveBroadcastFromListenerPort(buffer, size, mBroadcastPort);

		if (bytesReceived == -1)
		{
			std::cout << mUdp->GetLastError() << std::endl;
			return -1;
		}
		else if (bytesReceived >= sizeof(UPDATER_ACTION_MESSAGE) && IsPacketValid(buffer))
		{
			UPDATER_ACTION_MESSAGE msg = GetMessageFromBuffer(buffer);

			if (msg.action == ACTION_COMMAND::BOOT_INTERRUPT)
			{
				// Get the senders info
				std::string ip;
				int port;
				mUdp->GetLastSendersInfo(ip, port);
				mUdp->ConfigureThisClient("", 8080);
				mUdp->OpenUnicast();

				// Respond to sender with ack
				int rtn = SendAcknowledgement("127.0.0.1", 8080, MSG_TYPE::BOOT_INTERRUPT);
				if (rtn < 0)
				{
					std::cout << "[UPDATER] Failed to send broadcast response";
					return -1;
				}

				std::cout << "[UPDATER] Broadcast Ack sent to " + ip + ":" + std::to_string(port) + "\n";
				return 1;
			}
			else
			{
				// Not a boot interrupt message, disregard it. 
				// Do I want to send an ack here even if its not a boot interrupt message  ?
				continue;
			}
		}

		elapsed = mTimer->GetMSecTicks() - start;
	}

	// Default return
	return 0;
}

bool UnitUpdater::IsPacketValid(const uint8_t* buffer)
{
	if (buffer[0] == SYNC1 &&
		buffer[1] == SYNC2 &&
		buffer[2] == SYNC3 &&
		buffer[3] == SYNC4 &&
		buffer[4] == sizeof(UPDATER_ACTION_MESSAGE)
		)
	{
		UPDATER_ACTION_MESSAGE msg = GetMessageFromBuffer(buffer);

		switch (msg.action)
		{
		case ACTION_COMMAND::CLOSE:
		case ACTION_COMMAND::BOOT_INTERRUPT:
		case ACTION_COMMAND::GET_AS_BUILT:
		case ACTION_COMMAND::UPDATE_OFS:
		case ACTION_COMMAND::UPDATE_CONFIG:
		case ACTION_COMMAND::GET_LOG_NAMES:
		case ACTION_COMMAND::GET_SPECIFIC_LOG:
		case ACTION_COMMAND::GET_LAST_FLIGHT_LOG:
			return true;
		}
	}
	return false;
}

std::string UnitUpdater::SerializeResponseMsg(const RESPONSE_MSG& responseMsg)
{
	std::stringstream ss;

	// Serialize the structure members into a buffer
	ss.write(reinterpret_cast<const char*>(&responseMsg.header), sizeof(responseMsg.header));
	ss.write(reinterpret_cast<const char*>(&responseMsg.action), sizeof(responseMsg.action));
	ss.write(reinterpret_cast<const char*>(&responseMsg.status), sizeof(responseMsg.status));

	// Serialize the 'data' string separately
	size_t dataSize = responseMsg.data.size();
	ss.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
	ss.write(responseMsg.data.c_str(), dataSize);

	ss.write(reinterpret_cast<const char*>(&responseMsg.footer), sizeof(responseMsg.footer));

	// Get the serialized data as a string
	return ss.str();
}

UPDATER_ACTION_MESSAGE UnitUpdater::GetMessageFromBuffer(const uint8_t* buffer)
{
	UPDATER_ACTION_MESSAGE msg = { 0 };
	memcpy(&msg, buffer, sizeof(msg));
	return msg;
}

int UnitUpdater::SendAcknowledgement(const std::string ip, const int port, const MSG_TYPE type)
{
	int rtn = -1;
	UPDATER_ACTION_ACK msg = { SYNC1, SYNC2, SYNC3, SYNC4, sizeof(UPDATER_ACTION_MESSAGE), 0, ACKNOWLEDGE, 0 };

	switch (type)
	{
	case MSG_TYPE::BOOT_INTERRUPT:
		msg.action = BOOT_INTERRUPT;
		rtn = mUdp->SendUnicast(reinterpret_cast<char*>(&msg), sizeof(msg), ip, port);
		break;
	case MSG_TYPE::UPDATE_OFS:				break;
	case MSG_TYPE::UPDATE_CONFIG:			break;
	case MSG_TYPE::GET_LOG_NAMES:			break;
	case MSG_TYPE::GET_SPECIFIC_LOG:		break;
	case MSG_TYPE::GET_LAST_FLIGHT_LOG:		break;
	}

	return rtn;
}

void UnitUpdater::Close()
{
	mTimer->ReleaseInstance();
}