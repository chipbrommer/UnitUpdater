#pragma once

#include <iostream>
#include "tcp_server.h"
#include "udp_client.h"

class UnitUpdater
{
public:
    UnitUpdater();
    UnitUpdater(uint16_t broadcastPort, uint16_t serverPort);
    ~UnitUpdater();
    void ListenForBroadcast();
protected:
private:

};