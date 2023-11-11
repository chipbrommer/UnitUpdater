#include "UnitUpdater.h"


UnitUpdater::UnitUpdater() 
{
    std::cout << Essentials::Communications::UdpClientVersion;
    std::cout << Essentials::Communications::TcpServerVersion;
}

UnitUpdater::UnitUpdater(int broadcastPort, int serverPort) : UnitUpdater()
{
    this->broadcastPort = broadcastPort;
    this->serverPort = serverPort;
}

UnitUpdater::~UnitUpdater() {}

int UnitUpdater::Start()
{
    udp.AddBroadcastListener(broadcastPort);
    std::cout << Essentials::Communications::UdpClientVersion;

    return 0;
}

void UnitUpdater::ListenForBroadcast()
{

}