#pragma once

#include <iostream>
#include "tcp_server.h"
#include "udp_client.h"

constexpr double DEFAULT_TIMELENGTH = 1.0;

class UnitUpdater
{
public:
    UnitUpdater();
    UnitUpdater(int bPort, int tPort);
    ~UnitUpdater();
    int Setup(int bPort, int tPort);
    void SetMaxListeningTime(double timeout);
    int Start();
    void ListenForInterrupt();
protected:
private:
    double maxTimeLengthInSeconds;
    int broadcastPort;
    int serverPort;
    Essentials::Communications::UDP_Client* udp;
    Essentials::Communications::TCP_Server* tcp;
};