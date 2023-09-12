#include <algorithm>
#include <iostream>
#include "rate_limiter.h"

using namespace std::chrono;

CRateLimiter::CRateLimiter(double permitsPerSecond, double capacity)
{
    if (permitsPerSecond <= 0.0)
        throw std::runtime_error("CRateLimiter: permitsPerSecond param is invalid for CRateLimiter()");

    m_maxPermits = capacity;
    m_stableIntervalMicros = 1000000.0 / permitsPerSecond;
    m_storedPermits = capacity;  //0.0
    m_nextFreeTicketMicros = 0;
}

double CRateLimiter::Aquire(int permits)
{
    if (permits < 0)
        throw std::runtime_error("CRateLimiter: permits param is invalid for Aquire()");

    auto wait_time = reserve(permits);
    std::this_thread::sleep_for(microseconds(wait_time));

    return wait_time / 1000000.0;
}

bool CRateLimiter::TryAquire(int permits, int timeouts)
{
    auto now = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

    if (m_nextFreeTicketMicros > now + timeouts * 1000.0)
        return false;
    else
        Aquire(permits);

    return true;
}

double CRateLimiter::GetPermitsRate() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return 1000000.0 / m_stableIntervalMicros;
}

void CRateLimiter::SetPermitsRate(double permitsPerSecond)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (permitsPerSecond <= 0.0)
        throw std::runtime_error("CRateLimiter: permitsPerSecond param is invalid for SetRate()");

    m_stableIntervalMicros = 1000000.0 / permitsPerSecond;
}

void CRateLimiter::resync(unsigned long long nowMicros)
{
    if (nowMicros > m_nextFreeTicketMicros)
    {
        double new_permits = (nowMicros - m_nextFreeTicketMicros) / m_stableIntervalMicros; //本次新发放令牌数
        m_storedPermits = std::min(m_maxPermits, m_storedPermits + new_permits);
        m_nextFreeTicketMicros = nowMicros;
    }
}

unsigned long long CRateLimiter::reserve(double permits)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    //转为微秒为单位
    unsigned long long nowMicros = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
    resync(nowMicros);

    unsigned long long waitMicros = m_nextFreeTicketMicros - nowMicros; //如果令牌发放后积累令牌数够用，wait 就是 0，否则本次就要 wait

    double storedPermitsToSpend = std::min(permits, m_storedPermits); //本次消费的令牌数
    double freshPermits = permits - storedPermitsToSpend;  //是否需要透支消费

    unsigned long long freshPermitsMicros = (unsigned long long)(freshPermits * m_stableIntervalMicros);  //根据透支消费的令牌数反算需要的时间

    m_nextFreeTicketMicros += freshPermitsMicros; //如果这次有超前消费，则需要将下次发放令牌时间向后延到将来的某个时间
    m_storedPermits -= storedPermitsToSpend; //消费指定数量的令牌

    return std::max(waitMicros, 0ULL);
}
