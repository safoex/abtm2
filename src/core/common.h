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
#define EXCEPTION_WORD "__EXCEPTION__"

#define ALL_CHANGED_WORD "__ALL_CHANGED__"
#define ALL_MEMORY_WORD "__ALL_MEMORY__"
#define ON_EVERY_WORD "__ON_EVERY__"

#define ALL_CHANGED sample{{ALL_CHANGED_WORD, std::any()}}
#define ALL_MEMORY sample{{ALL_MEMORY_WORD, std::any()}}
#define ON_EVERY sample{{ON_EVERY_WORD, std::any()}}

#define LOAD_WORD "__LOAD__"
#define RESPONSE_WORD "__RESPONSE__"
#define TICKET_WORD "__TICKET__"
#define FORMAT_WORD "__FORMAT__"
#define YAML_WORD "__YAML__"
#define JSON_WORD "__JSON__"
#define YAML_STR_WORD "__YAML_STR__"
#define JSON_STR_WORD "__JSON_STR__"
#define KEY_WORD "__KEY__"


#define TICK_WORD "__TICK__"
#define TICK_SAMPLE {{TICK_WORD, nullptr}}
#define OK_WORD "__OK__"



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

    NodeState from_string(std::string const& ns) {
        if(ns.empty())
            return UNDEFINED;
        switch (ns[0]) {
            case 'S': return SUCCESS;
            case 'F': return FAILURE;
            case 'R': return RUNNING;
            default: return UNDEFINED;
        }
    }

    sample make_exception(std::string const& what, std::string ticket = "") {
        return {{EXCEPTION_WORD, std::string(what)}, {TICKET_WORD, ticket}};
    }

    std::string get_exception(sample const& s) {
        if(s.count(EXCEPTION_WORD)) {
            return std::any_cast<std::string>(s.at(EXCEPTION_WORD));
        }
        else return {};
    }

};

#endif //ABTM2_COMMON_H
