#include "UnitUpdater.h"


UnitUpdater::UnitUpdater() 
{
    broadcastPort = 0;
    serverPort = 0;
	maxTimeLengthInMSec = DEFAULT_TIMELENGTH_MSEC;
	udp = new Essentials::Communications::UDP_Client();
    tcp = new Essentials::Communications::TCP_Server();
	timer = Essentials::Utilities::Timer::GetInstance();
	std::cout << tcp->TcpServerVersion;
}

UnitUpdater::UnitUpdater(int bPort, int tPort) : UnitUpdater()
{
	Setup(broadcastPort, serverPort);
}

int UnitUpdater::Setup(int bPort, int tPort)
{
	broadcastPort = bPort;
	serverPort = tPort;

	// Setup the UDP client to look for a broadcast on the desired port
	udp->AddBroadcastListener(broadcastPort);
	// Setup TCP to serve on any network interface on the desired port
	//tcp->Configure("0.0.0.0",serverPort);

	// Default return
	return 0;
}

void UnitUpdater::SetMaxListeningTime(int mSecTimeout)
{
	maxTimeLengthInMSec = mSecTimeout;
}

UnitUpdater::~UnitUpdater() 
{
	Close();
}

int UnitUpdater::Start()
{
    udp->AddBroadcastListener(broadcastPort);

    return 0;
}

int UnitUpdater::ListenForInterrupt()
{
	char buffer[200];
	int size = sizeof(buffer);
	int start = timer->GetMSecTicks();
	int elapsed = 0;

	while (elapsed <= maxTimeLengthInMSec)
	{
		int bytesReceived = udp->ReceiveBroadcastFromListenerPort(buffer, size, broadcastPort);

		if (bytesReceived == -1)
		{
			std::cout << udp->GetLastError() << std::endl;
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

		elapsed = timer->GetMSecTicks() - start;
	}

	return 0;
}

void UnitUpdater::Close()
{
	timer->ReleaseInstance();
}