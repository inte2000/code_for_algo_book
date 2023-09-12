#pragma once

#include <mutex>
#include <chrono>

class CRateLimiter 
{
public:
    CRateLimiter() = delete;
    CRateLimiter(double permitsPerSecond, double capacity = 0.0);
    double Aquire(int permits = 1);
    bool TryAquire(int permits, int timeouts = 0);

    double GetPermitsRate() const;
    void SetPermitsRate(double permitsPerSecond);
protected:
    virtual void resync(unsigned long long nowMicros);
    virtual unsigned long long reserve(double permits);

private:

    mutable std::mutex m_mutex;
    double m_stableIntervalMicros;
    double m_maxPermits;
    double m_storedPermits;
    unsigned long long m_nextFreeTicketMicros;
};
