#ifndef SCHEDULERST_H
#define SCHEDULERST_H

#include <cstddef>
#include <cstdint>
#include <variant> // monostate

struct ThreadPolicyST
{
    using Lock = std::monostate;
    using Mutex = std::monostate;

    static bool isThreaded()
    {
        return false;
    }
};

class ThreadST;

struct SchedulerST
{
    explicit SchedulerST(std::ptrdiff_t)
    {}

    uint64_t launch(const auto& fn);
};

class ThreadST
{
public:
    void sync() const
    {}

    [[nodiscard]] bool accepted()
    {
        return !rejected();
    }

    [[nodiscard]] bool rejected()
    {
        return false;
    }

private:
    friend class SchedulerST;
};

uint64_t SchedulerST::launch(const auto& fn)
{
    ThreadST thread;
    return fn(thread);
}

using Scheduler = SchedulerST;
using ThreadPolicy = ThreadPolicyST;

#endif // SCHEDULERST_H
