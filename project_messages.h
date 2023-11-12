///////////////////////////////////////////////////////////////////////////////
//! @file		project_messages.h
//! @brief		UnitUpdater messages
//! @author		Chip Brommer
///////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////
//
//  Includes:
//          name                        reason included
//          --------------------        ---------------------------------------
#include <cstdint>                      // standard types
//
///////////////////////////////////////////////////////////////////////////////

#pragma pack(push, 1)

constexpr uint8_t   SYNC1 = 0x1A;
constexpr uint8_t   SYNC2 = 0xBA;
constexpr uint8_t   SYNC3 = 0xF1;
constexpr uint8_t   SYNC4 = 0xD5;
constexpr uint16_t  BOOT_INTERRUPT  = 0x12AB;
constexpr uint16_t  ACKNOWLEDGE     = 0xBA21;

enum ACTION_COMMAND : unsigned int
{
    UPDATE_OFS          = 0xD2C3B4A5,
    UPDATE_CONFIG       = 0xD2C3B4A6,
    GET_LAST_FLIGHT_LOG = 0xE1D3C5B7,
};

struct UPDATER_HEADER
{
    uint8_t         sync1;
    uint8_t         sync2;
    uint8_t         sync3;
    uint8_t         sync4;
    uint8_t         msgSize;
};

struct UPDATER_FOOTER
{
    uint8_t         checksum;
};

struct UPDATER_BOOT_INTERRUPT_MESSAGE
{
    UPDATER_HEADER  header;
    uint16_t        command;
    UPDATER_FOOTER  footer;
};

struct UPDATER_ACTION_MESSAGE
{
    UPDATER_HEADER  header;
    uint32_t        action;
    UPDATER_FOOTER  footer;
};

struct UPDATER_ACTION_ACK
{
    UPDATER_HEADER  header;
    uint32_t        action;
    uint16_t        ack;
    UPDATER_FOOTER  footer;
};

#pragma pack(pop)