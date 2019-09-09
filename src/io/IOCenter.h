//
// Created by safoex on 06.09.19.
//

#ifndef ABTM2_IOCENTER_H
#define ABTM2_IOCENTER_H

#include "IOInterface.h"
#include "core/exec/ExecutorInterface.h"
#include <queue>


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

    public:

        explicit IOCenter(ExecutorInterface *executor) : executor(executor) {
            step = 0;
        }

        InputFunction registerIOchannel(IOInterface *channel, unsigned int priority = 0) {
            channels[channel] = priority;

//            std::cout << "registering channel" << std::endl;
//            for (auto const &p: channel->trigger_vars) {
//                std::cout << p.first << std::endl;
//            }
//            std::cout << std::endl;

            if (channel->trigger_vars.count(ON_EVERY_WORD)) {
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

        void process() {
            while (!tasks.empty()) {
                auto task = tasks.top();
                tasks.pop();
                process_once(task.msg, task.direction, task.channel);
            }
        }

        sample sample_for_channel(IOInterface *ch, sample const &s) {
            auto rs = ch->required_vars;
            if (rs.count(ALL_MEMORY_WORD)) {
                sample r;
                r["memory"] = executor->memory;
                return r;
            } else if (rs.count(ALL_CHANGED_WORD)) {
                return s;
            } else return executor->memory->update(rs);
        }


        //process_once(sample, OUTPUT, nullptr) -> put <sample> into <tasks> for all channels
        //process_once(sample, OUTPUT, channel) -> push <sample> into channel->process()
        //process_once(sample, INPUT, nullptr) -> apply out = executor->callback() and process(out, OUTPUT)
        //process_once(sample, INPUT, channel) -> put <sample> into <tasks>

        void process_once(sample sample, IO_DIRECTION direction, IOInterface *channel) {
            std::lock_guard lockGuard(lock);

//            std::cout << " ----- process_once ------" << std::endl;
//            std::cout << (direction == IO_INPUT ? "INPUT" : "OUTPUT") << std::endl;
//            for (auto const& p : sample) std::cout << p.first << '\t';
//            std::cout << std::endl;


            //process(sample, IO_INPUT) -> apply out = executor->callback() and process(out, IO_OUTPUT)
            if (direction == IO_INPUT && channel == nullptr) {
                sample = executor->execute(sample);
                direction = IO_DIRECTION::IO_OUTPUT;
            }

            //process(sample, OUTPUT, channel) -> push <sample> into channel->process()
            if (direction == IO_OUTPUT && channel != nullptr) {
                sample = channel->process(sample);
                direction = IO_INPUT;
            }

            //process(sample, IO_OUTPUT, nullptr) -> put <sample> into <tasks> for all channels
            if (direction == IO_OUTPUT && channel == nullptr) {

//                std::cout << "------- OUTPUT sample ---------" << std::endl;
//                for (auto const &p: sample) {
//                    std::cout << p.first << '\t' << std::any_cast<std::string>(p.second) << std::endl;
//                }
//                std::cout << std::endl << "--------------" << std::endl;

                if (!sample.empty()) {
                    std::unordered_set<IOInterface *> selected_channels{};
                    for (auto const &kv: sample) {
                        if (vars_to_channels.count(kv.first)) {
                            auto const &channels_from_var = vars_to_channels[kv.first];
                            selected_channels.insert(channels_from_var.begin(), channels_from_var.end());
                        }
                    }
                    selected_channels.insert(all_vars_channels.begin(), all_vars_channels.end());

                    for (auto const &m: selected_channels)
                        tasks.push({step, channels[m], m, IO_OUTPUT, sample_for_channel(m, sample)});
                    step++;
                }
            }



            //process(sample, IO_INPUT, channel) -> process(sample, IO_INPUT) || might be changed in future
            if (direction == IO_INPUT && channel != nullptr) {
                tasks.push({step, channels[channel], nullptr, IO_INPUT, sample});
                step++;
            }

//            std::cout << " ------ process once ended-------------------" << std::endl;
        }

        //same as above, but after performs process_tasks()
        void process(const sample &sample, IO_DIRECTION direction, IOInterface *channel) {
            process_once(sample, direction, channel);
            process();
        }


    };

}

#endif //ABTM2_IOCENTER_H
