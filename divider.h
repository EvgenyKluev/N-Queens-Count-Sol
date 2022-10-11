#ifndef DIVIDER_H
#define DIVIDER_H

/* Function object that returns false one out of "parts" times. May be useful
 * (1) to investigate long-running program, (2) to apply pgo, or (3) to divide
 * work into several smaller parts.
 */
class Divider
{
public:
    Divider(int start = 0, int parts = 1)
        : counter_(start)
        , parts_(parts)
    {}

    bool operator() ()
    {
        if (++counter_ >= parts_)
            counter_ = 0;
        return counter_ != 0;
    }

private:
    int counter_;
    int parts_;
};

#endif // DIVIDER_H
