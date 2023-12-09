///////////////////////////////////////////////////////////////////////////////
//! @file		project_settings.h
//! @brief		UnitUpdater settings
//! @author		Chip Brommer
///////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////
//
//  Includes:
//          name                        reason included
//          --------------------        ---------------------------------------
#include <string>                       // strings
#include <iostream>                     // iostream
#include <fstream>                      // file stream
#include "nlohmann/json.hpp"            // json
//
///////////////////////////////////////////////////////////////////////////////

constexpr int DEFAULT_BROADCAST_TIMEOUT = 1000;
constexpr int DEFAULT_BROADCAST_PORT = 5800;
constexpr int DEFAULT_COMMS_PORT = 5801;
constexpr int MINIMUM_TIMEOUT = 1000;
constexpr int MINIMUM_PORT = 1024;
constexpr int MAXIMUM_PORT = 65535;

/// @brief A structure to represent a settings file
struct Settings 
{
    std::string ofsLocation;        // OFS location
    std::string asBuiltLocation;    // As-Built log location
    std::string sdcardLocation;     // Location of SD Card mounting folder
    int broadcastTimeoutMSec;       // Timeout for broadcast listening
    int broadcastPort;              // Port for broadcast listening
    int communicationPort;          // Port for direct communication

    // @brief Default Constructor
    Settings() : ofsLocation(""), asBuiltLocation(""), sdcardLocation(""), 
        broadcastTimeoutMSec(DEFAULT_BROADCAST_TIMEOUT), broadcastPort(DEFAULT_BROADCAST_PORT), communicationPort(DEFAULT_COMMS_PORT) {}

    /// @brief Constructor
    /// @param ofsLocation - location of the OFS 
    /// @param asBuiltLocation - location of the as-built log
    /// @param sdcardLocation - location of the sdcard mount folder
    /// @param broadcastTimeoutMSec - timeout for broadcast listening
    Settings(const std::string& ofsLocation, const std::string& asBuiltLocation, const std::string& sdcardLocation, 
        const int broadcastTimeoutMSec, const int broadcastPort, const int communicationPort)
        : ofsLocation(ofsLocation), asBuiltLocation(asBuiltLocation), sdcardLocation(sdcardLocation), 
        broadcastTimeoutMSec(broadcastTimeoutMSec), broadcastPort(broadcastPort), communicationPort(communicationPort)
    {
        // Ensure broadcastTimeoutMSec is at least 1000
        this->broadcastTimeoutMSec = (broadcastTimeoutMSec >= MINIMUM_TIMEOUT) ? broadcastTimeoutMSec : DEFAULT_BROADCAST_TIMEOUT;

        // Ensure ports are valid ports: 1024-65535
        if (broadcastPort < MINIMUM_PORT || broadcastPort > MAXIMUM_PORT) {
            this->broadcastPort = DEFAULT_BROADCAST_PORT;
        }
        else {
            this->broadcastPort = broadcastPort;
        }

        if (broadcastPort < MINIMUM_PORT || broadcastPort > MAXIMUM_PORT) {
            this->broadcastPort = DEFAULT_COMMS_PORT;
        }
        else {
            this->broadcastPort = broadcastPort;
        }
    }

    /// @brief Check if two settings structures are equal
    /// @param rhs - Right hand side object
    /// @return - true if equal
    bool operator==(const Settings& rhs) const {
        return (ofsLocation             == rhs.ofsLocation          &&
                asBuiltLocation         == rhs.asBuiltLocation      &&
                sdcardLocation          == rhs.sdcardLocation       &&
                broadcastTimeoutMSec    == rhs.broadcastTimeoutMSec &&
                broadcastPort           == rhs.broadcastPort        &&
                communicationPort       == rhs.communicationPort);
    }

    /// @brief Converts settings to json structure
    /// @return json structure of the settings
    nlohmann::json ToJson() const {
        nlohmann::json settingsJson;
        settingsJson["ofsLocation"] = ofsLocation;
        settingsJson["asBuiltLocation"] = asBuiltLocation;
        settingsJson["sdcardLocation"] = sdcardLocation;
        settingsJson["broadcastTimeoutMSec"] = broadcastTimeoutMSec;
        settingsJson["broadcastPort"] = broadcastPort;
        settingsJson["communicationPort"] = communicationPort;
        return settingsJson;
    }

    /// @brief Load the settings from json
    /// @param j - pointer to the json data
    void LoadFromJson(const nlohmann::json& j) 
    {
        try 
        {
            ofsLocation = j.at("ofsLocation").get<std::string>();
            asBuiltLocation = j.at("asBuiltLocation").get<std::string>();
            sdcardLocation = j.at("sdcardLocation").get<std::string>();
            broadcastTimeoutMSec = j.at("broadcastTimeoutMSec").get<int>();

            // Validate and set broadcastPort
            int tempBroadcastPort = j.at("broadcastPort").get<int>();
            if (tempBroadcastPort < MINIMUM_PORT || tempBroadcastPort > MAXIMUM_PORT) 
            {
                std::cout << "Settings: Loaded invalid broadcast port, setting default: " << DEFAULT_BROADCAST_PORT << std::endl;
                broadcastPort = DEFAULT_BROADCAST_PORT;
            }
            else 
            {
                broadcastPort = tempBroadcastPort;
            }

            // Validate and set communicationPort
            int tempCommunicationPort = j.at("communicationPort").get<int>();
            if (tempCommunicationPort < MINIMUM_PORT || tempCommunicationPort > MAXIMUM_PORT)
            {
                std::cout << "Settings: Loaded invalid communication port, setting default: " << DEFAULT_COMMS_PORT << std::endl;
                communicationPort = DEFAULT_COMMS_PORT;
            }
            else
            {
                communicationPort = tempCommunicationPort;
            }
        }
        catch (const std::exception& e) 
        {
            std::cerr << "Settings: Error loading from JSON: " << std::string(e.what()) << std::endl;
        }
    }

    /// @brief Save the settings as json file
    /// @param filePath - in - filepath to save to
    /// @return - true if successful
    bool SaveToJsonFile(const std::string& filePath) const 
    {
        try 
        {
            nlohmann::json settingsJson = ToJson();

            std::ofstream file(filePath);
            if (!file.is_open()) 
            {
                std::cerr << "Error opening file: " << filePath << std::endl;
                return false;
            }

            file << settingsJson << std::endl;
            file.close();
            return true;
        }
        catch (const std::exception& e) 
        {
            std::cerr << "Error saving settings: " << e.what() << std::endl;
            return false;
        }
    }

    /// @brief Print the structure contents to console
    void Print() const 
    {
        std::cout << "Settings:" << std::endl;
        std::cout << "\tofsLocation:          " << this->ofsLocation            << std::endl;
        std::cout << "\tasBuiltLocation:      " << this->asBuiltLocation        << std::endl;
        std::cout << "\tsdcardLocation:       " << this->sdcardLocation         << std::endl;
        std::cout << "\tbroadcastTimeoutMSec: " << this->broadcastTimeoutMSec   << std::endl;
        std::cout << "\tbroadcastPort:        " << this->broadcastPort          << std::endl;
        std::cout << "\tcommunicationPort:    " << this->communicationPort      << std::endl;
    }
};