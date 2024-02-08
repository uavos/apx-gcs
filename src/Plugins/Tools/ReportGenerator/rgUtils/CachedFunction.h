#pragma once

#include <functional>

template<class RetT>
struct CachedFunction
{
    using func_t = std::function<RetT()>;

    CachedFunction(func_t func)
        : m_func(func)
        , is_cached(false){};

    void clear_cache() { is_cached = false; };

    RetT operator()()
    {
        if (is_cached)
            return m_cached_v;
        return m_cached_v = m_func();
    }

private:
    func_t m_func;
    RetT m_cached_v;
    bool is_cached;
};
