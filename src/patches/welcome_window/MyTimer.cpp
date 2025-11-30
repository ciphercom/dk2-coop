//
// Created by DiaLight on 11/29/2025.
//

#include "MyTimer.h"

MyTimer::MyTimer() {
    s_use_qpc = QueryPerformanceFrequency(&s_frequency);
    //        timeBeginPeriod(1);
    //        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    // Create a high resolution timer
    hTimer = CreateWaitableTimerEx(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    // Then configure it
    LARGE_INTEGER dueTime;
    dueTime.QuadPart = 0; // Start timer immediately
    SetWaitableTimer(hTimer, &dueTime, 1 /*every 1ms*/, nullptr, nullptr, FALSE);
}

time_t MyTimer::now_ms() const {
    if (s_use_qpc) {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return (1000LL * now.QuadPart) / s_frequency.QuadPart;
    } else {
        return GetTickCount();
    }
}

void MyTimer::sleep(time_t ms) const {
    // I have no idea how to wait precise amount of time
    time_t end = now_ms() + ms;
    time_t left = ms;
    while(left > 0) {
        //            time_t s = now_ms();
        if(left > 5) {  // trying to save cpu time
            if(hTimer) {
                LARGE_INTEGER dueTime;
                dueTime.QuadPart = -10000;  // 1ms
                //                    dueTime.QuadPart = -1;
                SetWaitableTimer(hTimer, &dueTime, 0, nullptr, nullptr, FALSE);
                WaitForSingleObject(hTimer, INFINITE);
            } else {
                Sleep(1);
            }
        } else if(left > 3) {
            SwitchToThread();
        }
        //            time_t e = now_ms();
        //            if((e - s) > left) printf("[warn] oversleep %lld > %lld\n", e-s, left);
        left = end - now_ms();
    }
}
