#pragma once

#include <QtCore>

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
    virtual std::optional<QString> get_value(QString command) = 0;
};
