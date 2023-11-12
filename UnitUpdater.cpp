#include "UnitUpdater.h"


UnitUpdater::UnitUpdater() 
{
    mBroadcastPort = 0;
    mServerPort = 0;
	mMaxTimeLengthInMSec = DEFAULT_TIMELENGTH_MSEC;
	mUdp = new Essentials::Communications::UDP_Client();
    mTcp = new Essentials::Communications::TCP_Server();
	mTimer = Essentials::Utilities::Timer::GetInstance();
	std::cout << mTcp->TcpServerVersion;
}

UnitUpdater::UnitUpdater(int bPort, int tPort) : UnitUpdater()
{
	Setup(bPort, tPort);
}

int UnitUpdater::Setup(int bPort, int tPort)
{
	mBroadcastPort = bPort;
	mServerPort = tPort;

	// Setup TCP to serve on any network interface on the desired port
	mTcp->Configure("0.0.0.0",mServerPort);

	// Default return
	return 0;
}

void UnitUpdater::SetMaxListeningTime(int msecTimeout)
{
	mMaxTimeLengthInMSec = msecTimeout;
}

UnitUpdater::~UnitUpdater() 
{
	Close();
}

int UnitUpdater::Start()
{
	if (mServerPort == 0)
	{
		return -1;
	}

	// Start the tcp connection
	if (mTcp->Start() < 0)
	{
		std::cerr << "Failed to start the server." << std::endl;
		std::cout << mTcp->GetLastError();
		return 1;
	}

	for (;;)
	{
		//// Listen for incoming connections and handle message requests
		//int32_t clientSocket = mTcp->Accept(); 

		//if (clientSocket == -1)
		//{
		//	// Handle the error, log it, etc.
		//	std::cerr << "Error accepting incoming connection." << std::endl;
		//	continue;
		//}

		//// Handle the client connection in a separate thread or function
		//HandleClient(clientSocket);
	}

	mTcp->Stop();
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
	while (true/*elapsed <= mMaxTimeLengthInMSec*/)
	{
		int bytesReceived = mUdp->ReceiveBroadcastFromListenerPort(buffer, size, mBroadcastPort);

		if (bytesReceived == -1)
		{
			std::cout << mUdp->GetLastError() << std::endl;
			return -1;
		}
		else if (bytesReceived > 0)
		{
			if (bytesReceived >= sizeof(UPDATER_BOOT_INTERRUPT_MESSAGE))
			{
				if (IsPacketValid(buffer))
				{
					std::string ip;
					int port;
					mUdp->GetLastSendersInfo(ip, port);				// Get the senders info
					mUdp->ConfigureThisClient("", 8080);
					mUdp->OpenUnicast();
					int rtn = SendAcknowledgement("127.0.0.1", 8080, MSG_TYPE::BOOT);
					if (rtn < 0)
					{
						std::cout << "Failed to send response";
						return -1;
					}// Respond to sender with ack
					std::cout << "Ack sent to " + ip + ":" +std::to_string(port) + "\n";
					return 1;										// return success
				}
			}
		}

		elapsed = mTimer->GetMSecTicks() - start;
	}

	// Default return
	return 0;
}

bool UnitUpdater::IsPacketValid(uint8_t* buffer)
{
	if (buffer[0] == SYNC1 &&
		buffer[1] == SYNC2 &&
		buffer[2] == SYNC3 &&
		buffer[3] == SYNC4 &&
		buffer[4] == sizeof(UPDATER_BOOT_INTERRUPT_MESSAGE)
		)
	{
		UPDATER_BOOT_INTERRUPT_MESSAGE msg = {0};
		memcpy(&msg, buffer, sizeof(msg));
		if (msg.command == BOOT_INTERRUPT)
		{
			return true;
		}
	}
	return false;
}

int UnitUpdater::SendAcknowledgement(const std::string ip, const int port, const MSG_TYPE type)
{
	int rtn = -1;
	switch (type)
	{
	case MSG_TYPE::BOOT:
	{
		UPDATER_BOOT_INTERRUPT_MESSAGE msg = { SYNC1, SYNC2, SYNC3, SYNC4, sizeof(UPDATER_BOOT_INTERRUPT_MESSAGE), ACKNOWLEDGE, 0};
		rtn = mUdp->SendUnicast(reinterpret_cast<char*>(&msg), sizeof(msg), ip, port);
	}
	}
	return rtn;
}

void UnitUpdater::Close()
{
	mTimer->ReleaseInstance();
}