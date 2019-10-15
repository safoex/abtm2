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
#include <queue>
#include "yaml-cpp/yaml.h"

#include <iostream>
#include <chrono>
#include <cstdlib>

#define SCREAM(x) for(int i = 0; i < 20; i++) std::cout << x; std::cout << std::endl;
#define EXCEPTION_WORD std::string("__EXCEPTION__")
#define EMPTY_STR std::string()


#define ALL_CHANGED_WORD std::string("__ALL_CHANGED__")
#define ALL_MEMORY_WORD std::string("__ALL_MEMORY__")
#define ON_EVERY_WORD std::string("__ON_EVERY__")
#define STATE_WORD std::string("__STATE__")

#define ALL_CHANGED sample{{ALL_CHANGED_WORD, std::any()}}
#define ALL_MEMORY sample{{ALL_MEMORY_WORD, std::any()}}
#define ON_EVERY sample{{ON_EVERY_WORD, std::any()}}

#define LOAD_WORD std::string("__LOAD__")
#define UNLOAD_WORD std::string("__UNLOAD__")
#define RESPONSE_WORD std::string("__RESPONSE__")
#define TICKET_WORD std::string("__TICKET__")
#define FORMAT_WORD std::string("__FORMAT__")
#define YAML_WORD std::string("__YAML__")
#define JSON_WORD std::string("__JSON__")
#define YAML_STR_WORD std::string("__YAML_STR__")
#define JSON_STR_WORD std::string("__JSON_STR__")
#define KEY_WORD std::string("__KEY__")
#define BUILD_TREE_WORD std::string("__BUILD_TREE__")
#define YAML_TREE_WORD std::string("__YAML_TREE__")
#define ROS_YAML_TREE_WORD std::string("__ROS_YAML_TREE__")




#define TICK_WORD std::string("__TICK__")
#define TICK_SAMPLE {{TICK_WORD, nullptr}}
#define OK_WORD std::string("__OK__")

#define TRACE_WORD std::string("__TRACE__")
#define CHANGED_WORD std::string("__CHANGED__")

#define TREE_WORD std::string("__TREE__")
#define DOT_TREE_WORD std::string("__DOT_TREE__")
#define DOT_TREE_WORD_ROS std::string("__DOT_TREE_ROS_MSG__") //temporary
#define DOT_RT_TREE_WORD std::string("__DOT_RT_TREE__")

#define ROS_COMMAND_WORD "__ROS_COMMAND__"
#define ROS_COMMAND {{ROS_COMMAND_WORD, nullptr}}
#define ROS_EXCEPTION_WORD "__ROS_EXCEPTION__"
#define ROS_EXCEPTION {{ROS_EXCEPTION_WORD, nullptr}}
#define ROS_STATE_CHANGES_WORD "__ROS_STATE_CHANGES__"
#define ROS_VAR_CHANGES_WORD "__ROS_VAR_CHANGES__"
#define ROS_VAR_CHANGES_REQUEST_WORD "__ROS_VAR_CHANGES_REQUEST__"
#define ROS_CHANGE_FILE "__ROS_CHANGE_FILE__"
#define ROS_MAIN_AUTORELOAD_FILE "__ROS_MAIN_AUTORELOAD_FILE__"


#define RELOAD_COMMAND_WORD "__RELOAD_COMMAND__"
#define RELOAD_COMMAND {{RELOAD_COMMAND_WORD, nullptr}}


//#define DEBUG_MODE



namespace abtm {
    template<typename T> using dictOf = std::unordered_map<std::string, T>;
    typedef std::unordered_set<std::string> keys;
    typedef dictOf<std::any> sample;
    typedef std::queue<sample> trace;
    typedef std::function<sample(sample const&)> ExternalFunction;
    typedef std::function<void(sample const&)> InputFunction;
    typedef std::function<bool()> BFunction;
    typedef dictOf<YAML::Node> tree_rep;


    bool is_state_var(std::string const& key) {
        return key.size() >= 9 && key.substr(0,9) == STATE_WORD;
    }

    bool is_keyword(std::string const& key) {
        bool traditional_key =  key.size() >= 4 && key.substr(0,2) == "__" && key.substr(key.size()-2) == "__";
        return traditional_key && !is_state_var(key);
    }


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
            if(!is_keyword(k))
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
#ifdef DEBUG_MODE
        throw std::runtime_error(what);
#endif
        return {{EXCEPTION_WORD, std::string(what)}, {TICKET_WORD, ticket}};
    }

    std::string get_exception(sample const& s) {
        if(s.count(EXCEPTION_WORD)) {
            return std::any_cast<std::string>(s.at(EXCEPTION_WORD));
        }
        else return {};
    }

    bool ok_response(sample const& s) {
        return s.count(RESPONSE_WORD) && std::any_cast<std::string>(s.at(RESPONSE_WORD)) == OK_WORD;
    }

    std::string keys_of(sample const&s) {
        std::string ss;
        for(auto const& [k,v] : s) {
            ss += k + '\t';
        }
        return ss;
    }

    template<typename TimeT = std::chrono::microseconds>
    struct measure
    {
        template<typename F, typename ...Args>
        static typename TimeT::rep execution(F&& func, Args&&... args)
        {
            auto start = std::chrono::steady_clock::now();
            std::forward<decltype(func)>(func)(std::forward<Args>(args)...);
            auto duration = std::chrono::duration_cast< TimeT>
                    (std::chrono::steady_clock::now() - start);
            return duration.count();
        }
    };
}

#endif //ABTM2_COMMON_H
