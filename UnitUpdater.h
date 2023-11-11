#pragma once

#include <iostream>
#include "tcp_server.h"
#include "udp_client.h"
#include "timer.h"
#include "project_messages.h"

constexpr int DEFAULT_TIMELENGTH_MSEC = 1000;

enum class MSG_TYPE
{
    BOOT, 
};

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
    bool IsPacketValid(uint8_t* buffer);
    int SendAcknowledgement(const std::string ip, const int port, const MSG_TYPE type);
    int mLastError;
    int mMaxTimeLengthInMSec;
    int mBroadcastPort;
    int mServerPort;
    Essentials::Communications::UDP_Client* mUdp;
    Essentials::Communications::TCP_Server* mTcp;
    Essentials::Utilities::Timer*           mTimer;
};