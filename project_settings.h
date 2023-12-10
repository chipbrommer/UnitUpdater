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
constexpr int DEFAULT_BROADCAST_PORT    = 5800;
constexpr int DEFAULT_COMMS_PORT        = 5801;
constexpr int DEFAULT_CONNECTIONS_LIMIT = 3;
constexpr int MINIMUM_TIMEOUT           = 1000;
constexpr int MINIMUM_PORT              = 1024;
constexpr int MAXIMUM_PORT              = 65535;
constexpr int MINIMUM_CONNECTIONS       = 1;   

/// @brief A structure to represent a settings file
struct Settings 
{
    std::string ofsLocation;        // OFS location
    std::string asBuiltLocation;    // As-Built log location
    std::string sdcardLocation;     // Location of SD Card mounting folder
    int broadcastTimeoutMSec;       // Timeout for broadcast listening
    int broadcastPort;              // Port for broadcast listening
    int communicationPort;          // Port for direct communication
    int maximumConnections;         // Maximum number of connections for TCP server 

    // @brief Default Constructor
    Settings() : ofsLocation(""), asBuiltLocation(""), sdcardLocation(""), broadcastTimeoutMSec(DEFAULT_BROADCAST_TIMEOUT), 
        broadcastPort(DEFAULT_BROADCAST_PORT), communicationPort(DEFAULT_COMMS_PORT), maximumConnections(DEFAULT_CONNECTIONS_LIMIT) {}

    /// @brief Constructor
    /// @param ofsLocation - location of the OFS 
    /// @param asBuiltLocation - location of the as-built log
    /// @param sdcardLocation - location of the sdcard mount folder
    /// @param broadcastTimeoutMSec - timeout for broadcast listening
    Settings(const std::string& ofsLocation, const std::string& asBuiltLocation, const std::string& sdcardLocation, 
        const int broadcastTimeoutMSec, const int broadcastPort, const int communicationPort, const int maximumConnections)
        : ofsLocation(ofsLocation), asBuiltLocation(asBuiltLocation), sdcardLocation(sdcardLocation), 
        broadcastTimeoutMSec(broadcastTimeoutMSec), broadcastPort(broadcastPort), communicationPort(communicationPort), maximumConnections(maximumConnections)
    {
        // Ensure broadcastTimeoutMSec is at least 1000
        this->broadcastTimeoutMSec = (broadcastTimeoutMSec >= MINIMUM_TIMEOUT) ? broadcastTimeoutMSec : DEFAULT_BROADCAST_TIMEOUT;

        // Ensure ports are valid ports: 1024-65535
        this->broadcastPort     = (broadcastPort < MINIMUM_PORT || broadcastPort > MAXIMUM_PORT) ? DEFAULT_BROADCAST_PORT : broadcastPort;
        this->communicationPort = (communicationPort < MINIMUM_PORT || communicationPort > MAXIMUM_PORT) ? DEFAULT_COMMS_PORT : communicationPort;

        // Make sure maximum connections is above the minimum. 
        this->maximumConnections = (maximumConnections < MINIMUM_CONNECTIONS) ? DEFAULT_CONNECTIONS_LIMIT : maximumConnections;
    }

    /// @brief Check if two settings structures are equal
    /// @param rhs - Right hand side object
    /// @return - true if equal
    bool operator == (const Settings& rhs) const 
    {
        return (ofsLocation             == rhs.ofsLocation          &&
                asBuiltLocation         == rhs.asBuiltLocation      &&
                sdcardLocation          == rhs.sdcardLocation       &&
                broadcastTimeoutMSec    == rhs.broadcastTimeoutMSec &&
                broadcastPort           == rhs.broadcastPort        &&
                communicationPort       == rhs.communicationPort    &&
                maximumConnections      == rhs.maximumConnections);
    }

    /// @brief Converts settings to json structure
    /// @return json structure of the settings
    nlohmann::json ToJson() const 
    {
        nlohmann::json settingsJson;
        settingsJson["ofsLocation"] = ofsLocation;
        settingsJson["asBuiltLocation"] = asBuiltLocation;
        settingsJson["sdcardLocation"] = sdcardLocation;
        settingsJson["broadcastTimeoutMSec"] = broadcastTimeoutMSec;
        settingsJson["broadcastPort"] = broadcastPort;
        settingsJson["communicationPort"] = communicationPort;
        settingsJson["maximumConnections"] = maximumConnections;
        return settingsJson;
    }

    /// @brief Load the settings from json
    /// @param j - pointer to the json data
    void LoadFromJson(const nlohmann::json& j) 
    {
        try 
        {
            ofsLocation             = j.at("ofsLocation").get<std::string>();
            asBuiltLocation         = j.at("asBuiltLocation").get<std::string>();
            sdcardLocation          = j.at("sdcardLocation").get<std::string>();

            // Validate and set broadcastTimeoutMSec
            broadcastTimeoutMSec = j.at("broadcastTimeoutMSec").get<int>();
            if (broadcastTimeoutMSec < MINIMUM_TIMEOUT)
            {
                std::cout << "[SETTINGS] Loaded invalid broadcast timeout, setting default: " << DEFAULT_BROADCAST_TIMEOUT << std::endl;
                broadcastTimeoutMSec = DEFAULT_BROADCAST_TIMEOUT;
            }

            // Validate and set broadcastPort
            broadcastPort = j.at("broadcastPort").get<int>();
            if (broadcastPort < MINIMUM_PORT || broadcastPort > MAXIMUM_PORT)
            {
                std::cout << "[SETTINGS] Loaded invalid broadcast port, setting default: " << DEFAULT_BROADCAST_PORT << std::endl;
                broadcastPort = DEFAULT_BROADCAST_PORT;
            }

            // Validate and set communicationPort
            communicationPort = j.at("communicationPort").get<int>();
            if (communicationPort < MINIMUM_PORT || communicationPort > MAXIMUM_PORT)
            {
                std::cout << "[SETTINGS] Loaded invalid communication port, setting default: " << DEFAULT_COMMS_PORT << std::endl;
                communicationPort = DEFAULT_COMMS_PORT;
            }

            // Validate and set maximumConnections
            maximumConnections = j.at("maximumConnections").get<int>();
            if (maximumConnections < MINIMUM_CONNECTIONS)
            {
                std::cout << "[SETTINGS] Loaded invalid maximum connections, setting default: " << DEFAULT_CONNECTIONS_LIMIT << std::endl;
                maximumConnections = DEFAULT_CONNECTIONS_LIMIT;
            }
        }
        catch (const std::exception& e) 
        {
            std::cerr << "[SETTINGS] Error loading from JSON: " << std::string(e.what()) << std::endl;
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
                std::cerr << "[SETTINGS] Error opening file: " << filePath << std::endl;
                return false;
            }

            file << settingsJson << std::endl;
            file.close();
            return true;
        }
        catch (const std::exception& e) 
        {
            std::cerr << "[SETTINGS] Error saving settings: " << e.what() << std::endl;
            return false;
        }
    }

    /// @brief Print the structure contents to console
    void Print() const 
    {
        std::cout << "[SETTINGS]" << std::endl;
        std::cout << "\tofsLocation:          " << this->ofsLocation            << std::endl;
        std::cout << "\tasBuiltLocation:      " << this->asBuiltLocation        << std::endl;
        std::cout << "\tsdcardLocation:       " << this->sdcardLocation         << std::endl;
        std::cout << "\tbroadcastTimeoutMSec: " << this->broadcastTimeoutMSec   << std::endl;
        std::cout << "\tbroadcastPort:        " << this->broadcastPort          << std::endl;
        std::cout << "\tcommunicationPort:    " << this->communicationPort      << std::endl;
        std::cout << "\tmaximumConnections:   " << this->maximumConnections     << std::endl;
    }
};