#ifndef CFG_H
#define CFG_H

// This allows to tune performance from single place
struct Cfg
{
    int sieveCuts = 0;
    int matchChunkSize = 5;
    int matchGroupSize = 8;
    int matchMinSize = 40;
    bool bmiIntrin = false;
    bool prefetch = false;
};

#endif // CFG_H
