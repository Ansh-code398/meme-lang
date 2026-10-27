#ifndef STRING_INTERNER_H
#define STRING_INTERNER_H

#include <string>
#include <unordered_set>
#include <string_view>
#include <mutex>

class StringInterner
{
private:
    std::unordered_set<std::string> pool;
    std::mutex mtx;

    StringInterner() = default;

public:
    static StringInterner &instance()
    {
        static StringInterner instance;
        return instance;
    }

    std::string_view intern(const std::string &str)
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = pool.find(str);
        if (it != pool.end())
        {
            return *it;
        }
        auto inserted = pool.insert(str);
        return *(inserted.first);
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lock(mtx);
        return pool.size();
    }
};

#endif
