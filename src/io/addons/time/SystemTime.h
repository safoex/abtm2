//
// Created by safoex on 11.10.19.
//

#ifndef ABTM2_SYSTEMTIME_H
#define ABTM2_SYSTEMTIME_H

#include "io/IOInterface.h"

#include "core/common.h"
#include <chrono>

namespace abtm {
    class SystemTime : public IOInterface {
    protected:
        std::string time_var;
        InputFunction _clocker;
        unsigned freq_hz;
        std::thread separate_thread;
    public:
        SystemTime(std::string const& time_var, unsigned freq_hz = 1000) : IOInterface(), time_var(time_var), freq_hz(freq_hz) {

        }

        void start_in_this_thread(InputFunction const& clocker) {
            _clocker = clocker;
            std::chrono::microseconds period(int(1e6/freq_hz));
            while(true) {
                std::this_thread::sleep_for(period);
                double time = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
                clocker({{time_var, time}});
            }
        }

        void start_in_separate_thread(InputFunction const& clocker) {
            separate_thread = std::thread([this, clocker]() {this->start_in_this_thread(clocker);});
        }
    };
}

#endif //ABTM2_SYSTEMTIME_H
