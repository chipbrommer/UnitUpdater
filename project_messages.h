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

constexpr uint8_t   SYNC1       = 0x1A;
constexpr uint8_t   SYNC2       = 0xBA;
constexpr uint8_t   SYNC3       = 0xF1;
constexpr uint8_t   SYNC4       = 0xD5;
constexpr uint16_t  ACKNOWLEDGE = 0xBA21;
constexpr uint16_t  EOB         = 0xA5E1;

enum class MSG_TYPE
{
    BOOT_INTERRUPT,
    GET_AS_BUILT,
    UPDATE_OFS,
    UPDATE_CONFIG,
    GET_LOG_NAMES,
    GET_SPECIFIC_LOG,
    GET_LAST_FLIGHT_LOG,
};

enum ACTION_COMMAND : std::uint32_t
{
    BOOT_INTERRUPT      = 0xB3C3B4A1,
    GET_AS_BUILT        = 0xB4C3B4A2,
    UPDATE_OFS          = 0xD2C3B4A3,
    UPDATE_CONFIG       = 0xD3C3B4A4,
    GET_LOG_NAMES       = 0xC1C3B4A5,
    GET_SPECIFIC_LOG    = 0xC2C3B4A6,
    GET_LAST_FLIGHT_LOG = 0xC3C3B4A7,
    CLOSE               = 0xA4C3B4A8,
};

enum ACTION_STATUS : std::uint32_t
{
    SUCCESS = 0x0001A5E1,
    FAIL    = 0x0002A5E1,
};

struct UPDATER_HEADER
{
    uint8_t         sync1;
    uint8_t         sync2;
    uint8_t         sync3;
    uint8_t         sync4;
    uint32_t        msgSize;
};

struct UPDATER_FOOTER
{
    uint16_t        eob;
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