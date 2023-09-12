// token.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "rate_limiter.h"

int main()
{
/*
    CRateLimiter limiter(1000, 1000);

    //std::this_thread::sleep_for(std::chrono::seconds(1));

    double wait = limiter.Aquire(700);
    std::cout << "0: Aquire 700: need_sleep: " << wait << std::endl;
    wait = limiter.Aquire(500);
    std::cout << "1: Aquire 500: need_sleep: " << wait << std::endl;
    //std::this_thread::sleep_for(std::chrono::seconds(1));
    wait = limiter.Aquire(700);
    std::cout << "2: Aquire 700: need_sleep: " << wait << std::endl;
    wait = limiter.Aquire(1000);
    std::cout << "3: Aquire 1000: need_sleep: " << wait << std::endl;
*/

    CRateLimiter limiter(1);

    double wait = limiter.Aquire(1);
    std::cout << "0: Aquire 1: need_sleep: " << wait << std::endl;
    wait = limiter.Aquire(1);
    std::cout << "1: Aquire 1: need_sleep: " << wait << std::endl;
    wait = limiter.Aquire(1);
    std::cout << "2: Aquire 1: need_sleep: " << wait << std::endl;
    wait = limiter.Aquire(1);
    std::cout << "3: Aquire 1: need_sleep: " << wait << std::endl;
 

}

