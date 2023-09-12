// bucket.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <random>
#include <ctime>
#include <chrono>
#include <iomanip>

using sys_clock = std::chrono::system_clock;

std::atomic<bool> quit_mark(false);
std::default_random_engine re(std::time(nullptr));
//std::default_random_engine re(sys_clock::now().time_since_epoch().count());


class Bucket
{
public:
    Bucket() = delete;
    Bucket(int capacity)
    {
        m_capacity = capacity;
        m_qty = 0;
    }
    bool Put(void *data, int size)
    {
        std::lock_guard<std::mutex> lock(m_mtp);
        if ((m_qty + size) > m_capacity)
            return false;

        m_qty += size;
        return true;
    }
    int Take(void *data_buf, int size)
    {
        std::lock_guard<std::mutex> lock(m_mtp);
        if (m_qty > size)
        {
            m_qty -= size;
            return size;
        }
        else
        {
            int rms = m_qty;
            m_qty = 0;
            return rms;
        }
    }

protected:
    std::mutex m_mtp;
    int m_qty;
    int m_capacity;
};

void producer(Bucket& bkt)
{
    while (!quit_mark)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::uniform_int_distribution<unsigned int> ud(300, 650);
        char* data = nullptr; //数据，为了演示算法简单，Bucket 不处理数据，这里就简单设为 nullptr
        int psize = ud(re);
        if (!bkt.Put(data, psize))
        {
            std::cout << "thread: " << std::this_thread::get_id() << " fail to put " << psize << " data to bucket" << std::endl;
        }
    }
}

void SendInterface(void* pData, int count)
{
}
//发送限制每秒 1K Bytes
void sender(Bucket & bkt)
{
    char data[200];
    while (!quit_mark)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));//  200ms 发送一次，每次 200 
        std::time_t curTime = sys_clock::to_time_t(sys_clock::now());
        int tsize = bkt.Take(data, 200); //从桶中漏出数据，存在 data 中
        std::cout << "thread: " << std::this_thread::get_id() << " send out " << tsize 
                  << " data at: " << std::put_time(std::localtime(&curTime), "%F, %T") << std::endl;


        //向外部接口发送数据
        SendInterface(data, tsize);
    }
}

void demo_stop()
{
    std::this_thread::sleep_for(std::chrono::seconds(10));
    quit_mark = true;
}

int main()
{
    Bucket bucket(50000);
    std::thread st(sender, std::ref(bucket));
    std::thread pt1(producer, std::ref(bucket));
    std::thread pt2(producer, std::ref(bucket));
    std::thread stop_demo(demo_stop);

    st.join();
    pt1.join();
    pt2.join();
    stop_demo.join();

    std::cout << "Hello World!\n";
}

