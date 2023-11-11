#pragma once

#include <iostream>
#include "tcp_server.h"
#include "udp_client.h"

class UnitUpdater
{
public:
    UnitUpdater();
    UnitUpdater(int broadcastPort, int serverPort);
    ~UnitUpdater();
    int Start();
    void ListenForBroadcast();
protected:
private:
    int broadcastPort;
    int serverPort;
    Essentials::Communications::UDP_Client udp;
    Essentials::Communications::TCP_Server tcp;
};