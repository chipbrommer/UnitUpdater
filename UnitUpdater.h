﻿#pragma once

#include <iostream>
#include "tcp_server.h"
#include "udp_client.h"
#include "timer.h"
#include "project_messages.h"
#include "project_settings.h"

constexpr int DEFAULT_TIMELENGTH_MSEC = 1000;

class UnitUpdater
{
public:
    UnitUpdater();
    UnitUpdater(int bPort, int tPort);
    ~UnitUpdater();
    int     Setup(std::string filepath, int preferredBroadcastPort = 0, int preferredCommsPort = 0);
    void    SetMaxBroadcastListeningTime(int mSecTimeout);
    int     StartServer();
    int     HandleMessage(const int clientFD, const std::string& msg);
    int     ListenForInterrupt();
    void    Close();
protected:
private:
    bool    IsPacketValid(const uint8_t* buffer);
    std::string SerializeResponseMsg(const RESPONSE_MSG& msg);
    UPDATER_ACTION_MESSAGE GetMessageFromBuffer(const uint8_t* buffer);
    int     SendAcknowledgement(const std::string ip, const int port, const MSG_TYPE type);

    int     mLastError;
    int     mMaxBroadcastListeningTimeInMSec;
    int     mBroadcastPort;
    int     mServerPort;
    bool    mCloseRequested;
    bool    mUpdateInProgress;

    Essentials::Communications::UDP_Client* mUdp;
    Essentials::Communications::TCP_Server* mTcp;
    Essentials::Utilities::Timer*           mTimer;
    Settings                                mSettings;
};