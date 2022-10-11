#ifndef FREEZE_H
#define FREEZE_H

#include <vector>

#include "scheduler.h"

/* Merges mutable thread-private (Sieve) objects into immutable shared one.
 * Also handles single-threaded case.
 *
 * Template parameters:
 * Obj - type of the object
 * Tag - disambiguation tag to insure uniquity of every Freeze instance
 */
template<class Obj, class Tag>
class Freeze
{
public:
    const Obj& getObj() const&
    {
        return obj_;
    }

    void reg(Obj* ptr)
    {
        ThreadPolicy::Lock lk(mutex_);
        ptrs_.push_back(ptr);
    }

    void freeze(auto* thread)
    {
        if (ThreadPolicy::isThreaded())
            obj_.pull(ptrs_, thread);
        else
            ptrs_.front()->closePatterns();
    }

    void clear()
    {
        if (ThreadPolicy::isThreaded())
            obj_.clear();
        else
            ptrs_.front()->clear();
    }

    void shrink()
    {
        if (ThreadPolicy::isThreaded())
            obj_.shrink(); // relinquish memory for possibly unused object
        // else keep memory allocated, it will be reused
    }

private:
    Obj obj_;
    std::vector<Obj*> ptrs_;
    mutable ThreadPolicy::Mutex mutex_;
};

#endif // FREEZE_H
