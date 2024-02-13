#pragma once

#include <functional>

/**
 * @brief Caches the return value of a parameterless function
 * 
 * @param RetT Return type of function
 */
template<class RetT>
struct ParamlessCachedFunction
{
    using func_t = std::function<RetT()>;

    ParamlessCachedFunction(func_t func)
        : m_func(func)
        , m_is_cached(false){};

    void clearCache() { m_is_cached = false; };

    RetT operator()()
    {
        if (m_is_cached)
            return m_cached_v;
        return m_cached_v = m_func();
    }

private:
    func_t m_func;
    RetT m_cached_v;
    bool m_is_cached;
};
