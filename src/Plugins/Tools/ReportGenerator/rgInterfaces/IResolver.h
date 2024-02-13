#pragma once

#include <QtCore>

namespace ReportGenerator {

/**
 * @brief Interface for Resolvers
 * 
 */
class IResolver
{
public:
    /**
     * @brief Used to parse and return needed result from command
     * 
     * @param command 
     * @return std::optional<QString> if no command found, then std::nullopt returned
     */
    virtual std::optional<QString> getValue(QString command) = 0;
};

}; // namespace ReportGenerator
