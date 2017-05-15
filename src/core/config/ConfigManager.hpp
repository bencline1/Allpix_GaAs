/**
 * @file
 * @brief Interface to the main configuration and its normal and special sections
 * @copyright MIT License
 */

#ifndef ALLPIX_CONFIG_MANAGER_H
#define ALLPIX_CONFIG_MANAGER_H

#include <set>
#include <string>
#include <vector>

#include "ConfigReader.hpp"
#include "Configuration.hpp"

namespace allpix {

    /**
     * @ingroup Managers
     * @brief Manager responsible for loading and providing access to the main configuration
     *
     * The main configuration is the single most important source of configuration. It is split up in:
     * - Global headers that are combined into a single global (not module specific) configuration
     * - Ignored headers that are not used at all (mainly useful for debugging)
     * - All other headers representing all modules that have to be instantiated by the ModuleManager
     *
     * Configuration sections are always case-sensitive.
     */
    class ConfigManager {
    public:
        /**
         * @brief Construct the configuration manager
         * @param file_name Path to the main configuration file
         */
        explicit ConfigManager(std::string file_name);

        /// @{
        /**
         * @brief Copying the manager is not allowed
         */
        ConfigManager(const ConfigManager&) = delete;
        ConfigManager& operator=(const ConfigManager&) = delete;
        /// @}

        /**
         * @brief Set the name of the global header and add to the global names
         * @param name Name of a global header that should be used as the name
         */
        // TODO [doc] Should only set the name and do not add it
        void setGlobalHeaderName(std::string name);
        /**
         * @brief Add a global header name
         * @param name Name of a global header section
         */
        // TODO [doc] Rename to addGlobalHeader
        void addGlobalHeaderName(std::string name);

        /**
         * @brief Get the global configuration
         * @return Global configuration
         */
        Configuration getGlobalConfiguration();

        /**
         * @brief Add a header name to fully ignore
         * @param name Name of a header to ignore
         */
        // TODO [doc] Rename to ignoreHeader
        void addIgnoreHeaderName(std::string name);

        /**
         * @brief Get all configurations that are not global or ignored
         * @return List of all normal configurations
         */
        std::vector<Configuration> getConfigurations() const;

    private:
        std::string file_name_;

        ConfigReader reader_;

        std::string global_default_name_;
        std::set<std::string> global_names_;
        std::set<std::string> ignore_names_;
    };
} // namespace allpix

#endif /* ALLPIX_CONFIG_MANAGER_H */
