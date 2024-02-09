#pragma once

#include <functional>

template<class RetT>
struct CachedFunction
{
    using func_t = std::function<RetT()>;

    CachedFunction(func_t func)
        : m_func(func)
        , m_is_cached(false){};

    void clear_cache() { m_is_cached = false; };

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
