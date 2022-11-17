#ifndef SCHEDULERMT_H
#define SCHEDULERMT_H

#include <atomic>
#include <barrier>
#include <cstddef>
#include <cstdint>
#include <future>
#include <mutex>
#include <vector>

struct ThreadPolicyMT
{
    template<class... MutexTypes>
    using Lock = std::scoped_lock<MutexTypes...>;
    using Mutex = std::mutex;

    static bool isThreaded()
    {
        return true;
    }
};

class SchedulerMT
{
public:
    explicit SchedulerMT(std::ptrdiff_t workerCount)
        : workerCount_{workerCount}
        , barrier_(workerCount)
    {}

    uint64_t launch(const auto& fn);

private:
    friend class ThreadMT;
    std::ptrdiff_t workerCount_;
    std::barrier<> barrier_;
    std::atomic_uint64_t work_ = 2;
};

class ThreadMT
{
public:
    void sync() const
    {
        scheduler_->barrier_.arrive_and_wait();
    }

    [[nodiscard]] bool accepted()
    {
        return !rejected();
    }

    // Requires one ignored call at start (see launch())
    [[nodiscard]] bool rejected()
    {
        if (++curr_ == next_)
        {
            next_ = scheduler_->work_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }

        return true;
    }

private:
    explicit ThreadMT(SchedulerMT* scheduler)
        : scheduler_{scheduler}
    {}

    friend class SchedulerMT;
    SchedulerMT* scheduler_;
    uint64_t curr_ = 0;
    uint64_t next_ = 1;
};

uint64_t SchedulerMT::launch(const auto& fn)
{
    std::vector<std::future<uint64_t>> fut(workerCount_);
    uint64_t total = 0;

    auto launcher = [&] {
        ThreadMT thread{this};
        (void)thread.rejected(); // this must be called once and ignored
        return fn(thread);
    };

    for (auto& slot: fut)
        slot = std::async(std::launch::async, launcher);

    for (auto& slot: fut)
        total += slot.get();

    return total;
}

using Scheduler = SchedulerMT;
using ThreadPolicy = ThreadPolicyMT;

#endif // SCHEDULERMT_H
