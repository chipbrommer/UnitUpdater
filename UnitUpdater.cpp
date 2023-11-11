#include "UnitUpdater.h"


UnitUpdater::UnitUpdater() 
{
    broadcastPort = 0;
    serverPort = 0;
	maxTimeLengthInSeconds = DEFAULT_TIMELENGTH;
	udp = new Essentials::Communications::UDP_Client();
    tcp = new Essentials::Communications::TCP_Server();
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

void UnitUpdater::SetMaxListeningTime(double timeout)
{
	maxTimeLengthInSeconds = timeout;
}

UnitUpdater::~UnitUpdater() {}

int UnitUpdater::Start()
{
    udp->AddBroadcastListener(broadcastPort);

    return 0;
}

void UnitUpdater::ListenForInterrupt()
{
	char buffer[200];
	int size = sizeof(buffer);
	bool interruptReceived = false;
	double time = 0;

	while (!interruptReceived && time <= maxTimeLengthInSeconds) 
	{

		int bytesReceived = udp->ReceiveBroadcastFromListenerPort(buffer, size, broadcastPort);

		if (bytesReceived == -1)
		{
			std::cout << udp->GetLastError() << std::endl;
		}
		else if (bytesReceived > 0)
		{
			// Verify the packet is what we desire
		}
		else
		{
			std::cout << "No Data." << std::endl;
		}
	}
}