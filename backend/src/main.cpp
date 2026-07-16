#include <drogon/drogon.h>
#include <iostream>
#include <fstream>
#include "config/config_manager.h"
#include "services/character_service.h"
#include "services/group_service.h"

using namespace drogon;

int main()
{
    std::ofstream log("chronochat_startup.log");
    log << "ChronoChat main() entered!" << std::endl;
    log.flush();

    std::cout << "ChronoChat Backend Starting..." << std::endl;
    std::cout.flush();

    // Initialize ConfigManager (uses backend/data/chronochat.db)
    log << "Initializing ConfigManager..." << std::endl;
    if (!chronochat::ConfigManager::instance().initialize("../../data/chronochat.db")) {
        log << "ConfigManager init failed!" << std::endl;
        std::cerr << "Failed to initialize ConfigManager!" << std::endl;
        return 1;
    }
    log << "ConfigManager OK" << std::endl;

    // Initialize CharacterService (uses root data/ for files, backend/data/ for DB)
    log << "Initializing CharacterService..." << std::endl;
    if (!chronochat::CharacterService::instance().initialize(
            "../../../data",              // character files at ChronoChat/data/
            "../../data/chronochat.db"))   // metadata in same DB
    {
        log << "CharacterService init failed!" << std::endl;
        std::cerr << "Failed to initialize CharacterService!" << std::endl;
        return 1;
    }
    log << "CharacterService OK" << std::endl;

    // Initialize GroupService
    log << "Initializing GroupService..." << std::endl;
    if (!chronochat::GroupService::instance().initialize(
            "../../../data",              // group files at ChronoChat/data/
            "../../data/chronochat.db"))   // metadata in same DB
    {
        log << "GroupService init failed!" << std::endl;
        std::cerr << "Failed to initialize GroupService!" << std::endl;
        return 1;
    }
    log << "GroupService OK" << std::endl;

    try {
        log << "Loading config.json..." << std::endl;
        log.flush();
        app().loadConfigFile("config.json");
        log << "Config loaded OK" << std::endl;
        log.flush();
    } catch (const std::exception& e) {
        log << "Config error: " << e.what() << std::endl;
        log.close();
        return 1;
    }

    app().setLogLevel(trantor::Logger::kInfo);

    log << "Calling app().run()..." << std::endl;
    log.flush();

    std::cout << "Starting server on port 8080..." << std::endl;
    std::cout.flush();

    app().run();

    log << "Server stopped." << std::endl;
    log.close();
    return 0;
}
