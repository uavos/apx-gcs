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

protected:
    Singleton() = default;
    Singleton(const Singleton &) = delete;
    Singleton(Singleton &&) = delete;
    Singleton &operator=(const Singleton &) = delete;
    Singleton &operator=(Singleton &&) = delete;
};
