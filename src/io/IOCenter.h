//
// Created by safoex on 06.09.19.
//

#ifndef ABTM2_IOCENTER_H
#define ABTM2_IOCENTER_H

#include "IOInterface.h"
#include "IOExecutor.h"
#include "core/exec/ExecutorInterface.h"
#include <queue>

#define IO_DEBUG false

namespace abtm {
    enum IO_DIRECTION {
        IO_INPUT,
        IO_OUTPUT
    };

    class IOTask {
    public:
//        unsigned long long ticket;
//        unsigned channel_priority;
        std::pair<unsigned long long, unsigned> ticket_and_priority;
        IOInterface *channel;
        IO_DIRECTION direction;
        sample msg;

        bool operator<(IOTask const &rhs) const {
            return ticket_and_priority < rhs.ticket_and_priority;
        }

        IOTask(unsigned long long ticket, unsigned channel_priority, IOInterface *channel, IO_DIRECTION direction,
               sample const& msg)
                : ticket_and_priority(ticket, channel_priority), channel(channel), direction(direction), msg(msg) {}

    };

    class IOCenter {
    public:
        ExecutorInterface *executor;
        IOExecutor ioExecutor;

        // vector< <priority, channel> >
        std::unordered_map<IOInterface *, unsigned> channels;

        //quickly find whether we need to feed the channel
        dictOf<std::unordered_set<IOInterface *>> vars_to_channels;
        std::unordered_set<IOInterface *> all_vars_channels;


        // step accumulates number of processed samples and used as "time when sample executed"
        unsigned long long step;

        // < <time when put into the queue, priority>, <channel, <output or input,  sample> > > >

        std::priority_queue<IOTask> tasks;

        //thread safety
        std::mutex lock;
        bool holded;

    public:

        explicit IOCenter(ExecutorInterface *executor) : executor(executor), ioExecutor(executor) {
            step = 0;
            registerIOchannel(&ioExecutor);
            holded = false;
        }

        void unregisterIOchannel(IOInterface* channel) {
            if(all_vars_channels.count(channel)) {
                all_vars_channels.erase(channel);
            }
            for(auto const& [k,v]: vars_to_channels) {
                if(v.count(channel))
                    vars_to_channels[k].erase(channel);
            }
            if(channels.count(channel))
                channels.erase(channel);
        }

        InputFunction registerIOchannel(IOInterface *channel, unsigned int priority = 0) {
            channels[channel] = priority;

            if(IO_DEBUG)
            {
                for (auto[k, v]: channel->trigger_vars) {
                    std::cout << k << '\t';
                }
                std::cout << "\n-=-=-=-=-=-\n";
            }
            if (channel->trigger_vars.count(ON_EVERY_WORD) || channel->trigger_vars.count(ALL_CHANGED_WORD)) {
                all_vars_channels.insert(channel);
            } else {
                for (auto const &kv: channel->trigger_vars) {
                    vars_to_channels[kv.first].insert(channel);
                }
            }
            return [this, channel](sample const &s) {
                this->process(s, IO_INPUT, channel);
            };
        }

        void stop() {
            std::lock_guard lockGuard(lock);
            holded = true;
        }

        void process() {
            std::lock_guard lockGuard(lock);
            if(!holded)
            while (!tasks.empty()) {
                auto task = tasks.top();
                tasks.pop();
                process_once_v2(task.msg, task.direction, task.channel, false);
            }
        }

        sample sample_for_channel(IOInterface *ch, sample const &s) {
            // add support for executor as nullptr
            auto rs = ch->required_vars;
            for(auto [k,v] : rs) {
                if(s.count(k)) {
                    rs[k] = s.at(k);
                }
            }
            if (rs.count(ALL_MEMORY_WORD)) {
                sample r;
                r["memory"] = executor->memory;
                return r;
            } else if (rs.count(ALL_CHANGED_WORD)) {
                return s;
            } else return executor->memory->update(rs);
        }


        // INPUT - from channel
        // OUTPUT - to channel

        void process_once_v2(sample const& s, IO_DIRECTION direction, IOInterface *channel, bool need_to_lock = true) {
            if(need_to_lock) {
                std::lock_guard lockGuard(lock);
                if(!holded)
                    process_once_v2_no_lock(s, direction, channel);
            }
            else {
                if(!holded)
                    process_once_v2_no_lock(s, direction, channel);
            }
        }

        void process_once_v2_no_lock(sample const& s, IO_DIRECTION direction, IOInterface *channel) {
            if(IO_DEBUG)
            {
                if (channel) {
                    for (auto const&[k, v]: channel->trigger_vars)
                        std::cout << k << '\t';
                }
                std::cout << (direction == IO_OUTPUT ? "OUTPUT" : "INPUT") << '\t';
                for (auto const&[k, v]: s)
                    std::cout << k << '\t';
                std::cout << std::endl;
            }
            if(direction == IO_INPUT) {
                if(IO_DEBUG)
                {
                    std::cout << "============== IO_INPUT ==============\n";
                    for (auto[k, v]: s) {
                        std::cout << k << '\t';
                    }
                    std::cout << std::endl;
                }
                auto selected_channels = get_channels_for_sample(s);
                selected_channels.erase(channel);
                for (auto const &m: selected_channels)
                    tasks.push({step, channels[m], m, IO_OUTPUT, sample_for_channel(m, s)});
                step++;
            }
            else {

                if (IO_DEBUG)
                {
                    std::cout << "============== IO_OUTPUT ==============\n";
                    for (auto[k, v]: s) {
                        std::cout << k << '\t';
                    }
                    std::cout << std::endl;
                }
                sample res;
#ifndef DEBUG_MODE
                try {
#endif
                    res = channel->process(s);
#ifndef DEBUG_MODE
                }
                catch(std::exception& e) {
                    res = make_exception(e.what());
                }
#endif
                if(IO_DEBUG)
                    if(!get_exception(res).empty())
                        std::cout << get_exception(res) << std::endl;
                if(!res.empty()) {
                    tasks.push({step++, channels[channel], channel, IO_INPUT, res});
                }
            }
            if(IO_DEBUG) {
                std::cout << "\t\tout" << std::endl;
            }
        }

        std::unordered_set<IOInterface*> get_channels_for_sample(sample const& s) {
            std::unordered_set<IOInterface *> selected_channels{};
            for (auto const &kv: s) {
                if (vars_to_channels.count(kv.first)) {
                    auto const &channels_from_var = vars_to_channels[kv.first];
                    selected_channels.insert(channels_from_var.begin(), channels_from_var.end());
                }
            }
            selected_channels.insert(all_vars_channels.begin(), all_vars_channels.end());

            if(IO_DEBUG)
            {
                for (IOInterface *sc: selected_channels) {
                    for (auto[k, v]: sc->trigger_vars)
                        std::cout << k << '\t';
                    std::cout << '\n';
                }
                std::cout << "=-=-=-=-=!!=-=-=-=\n";
            }

            return selected_channels;
        }

        //same as above, but after performs process_tasks()
        void process(const sample &sample, IO_DIRECTION direction, IOInterface *channel) {
            if(channels.count(channel) || channel == nullptr) {
                process_once_v2(sample, direction, channel);
                process();
            }
        }


    };

}

#endif //ABTM2_IOCENTER_H
