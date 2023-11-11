#pragma once

#include <iostream>
#include "tcp_server.h"
#include "udp_client.h"
#include "timer.h"

constexpr int DEFAULT_TIMELENGTH_MSEC = 1000;

class UnitUpdater
{
public:
    UnitUpdater();
    UnitUpdater(int bPort, int tPort);
    ~UnitUpdater();
    int Setup(int bPort, int tPort);
    void SetMaxListeningTime(int mSecTimeout);
    int Start();
    int ListenForInterrupt();
    void Close();
protected:
private:
    int maxTimeLengthInMSec;
    int broadcastPort;
    int serverPort;
    Essentials::Communications::UDP_Client* udp;
    Essentials::Communications::TCP_Server* tcp;
    Essentials::Utilities::Timer*           timer;
};