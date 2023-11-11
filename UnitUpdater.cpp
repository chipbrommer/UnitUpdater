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

	// Setup the UDP client to look for a broadcast on the desired port
	//mUdp->AddBroadcastListener(mBroadcastPort);
	// Setup TCP to serve on any network interface on the desired port
	//tcp->Configure("0.0.0.0",serverPort);

	// Default return
	return 0;
}

void UnitUpdater::SetMaxListeningTime(int mSecTimeout)
{
	mMaxTimeLengthInMSec = mSecTimeout;
}

UnitUpdater::~UnitUpdater() 
{
	Close();
}

int UnitUpdater::Start()
{
	if (mBroadcastPort == 0 || mServerPort == 0)
	{
		return -1;
	}

    return mUdp->AddBroadcastListener(mBroadcastPort);
}

int UnitUpdater::ListenForInterrupt()
{
	char buffer[200];
	int size = sizeof(buffer);
	int start = mTimer->GetMSecTicks();
	int elapsed = 0;

	while (elapsed <= mMaxTimeLengthInMSec)
	{
		int bytesReceived = mUdp->ReceiveBroadcastFromListenerPort(buffer, size, mBroadcastPort);

		if (bytesReceived == -1)
		{
			std::cout << mUdp->GetLastError() << std::endl;
			break;
		}
		else if (bytesReceived > 0)
		{
			// Verify the packet is what we desire
			bool packetReceived = false;

			// @todo - check for packet here. 

			// handle packet reception
			if (packetReceived)
			{
				return -1;
			}
		}

		elapsed = mTimer->GetMSecTicks() - start;
	}

	return 0;
}

void UnitUpdater::Close()
{
	mTimer->ReleaseInstance();
}