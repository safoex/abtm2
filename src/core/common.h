//
// Created by safoex on 03.09.19.
//

#ifndef ABTM2_COMMON_H
#define ABTM2_COMMON_H


#include <unordered_map>
#include <unordered_set>
#include <any>
#include <functional>
#include <set>
#include <fstream>
#include <sstream>
#include <iostream>
#include <mutex>

#define SCREAM(x) for(int i = 0; i < 20; i++) std::cout << x; std::cout << std::endl;

namespace abtm {
    template<typename T> using dictOf = std::unordered_map<std::string, T>;
    typedef std::unordered_set<std::string> keys;
    typedef dictOf<std::any> sample;
    typedef std::function<sample(sample const&)> ExternalFunction;
    typedef std::function<void(sample const&)> InputFunction;
    typedef std::function<bool()> BFunction;

    template <typename T>
    sample make_sample(std::initializer_list<std::pair<std::string, T>> il) {
        sample s;
        for(auto const& [k,v]: il) {
            s.insert({k, std::any(v)});
        }
        return s;
    }

    template <typename T>
    sample make_sample(dictOf <T> const& d) {
        sample s;
        for(auto const& [k,v]: d) {
            s.insert({k, std::any(v)});
        }
        return s;
    }

    template <typename T>
    dictOf <T> from_sample(sample const& s) {
        dictOf <T> x;
        for(auto const& [k,v]: s) {
            x[k] = std::any_cast<T>(v);
        }
        return x;
    }

    enum NodeState {
        RUNNING,
        SUCCESS,
        FAILURE,
        UNDEFINED
    };

    std::string to_string(abtm::NodeState ns) {
        switch (ns) {
            case abtm::SUCCESS: return "SUCCESS";
            case abtm::FAILURE: return "FAILURE";
            case abtm::RUNNING: return "RUNNING";
            default: return "UNDEFINED";
        }
    }

};

#endif //ABTM2_COMMON_H
