#pragma once

#include <type_traits>

template<typename T>
class Singleton
{
public:
    static T &getInstance()
    {
        static T instance;
        return instance;
    }
};
