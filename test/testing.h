//
// Created by safoex on 06.09.19.
//

#ifndef ABTM2_TESTING_H
#define ABTM2_TESTING_H

#include <iostream>
#include <string>

#define COLORED_VIEW

std::string fill(std::string const& s, int symb = 60) {
    if(s.size() < symb) {
        return s + std::string(symb-s.size(), ' ');
    }
    return s;
}

#ifdef COLORED_VIEW

#define PASSED "\033[1;36;mPASSED\033[0m"
#define FAILED "\033[1;31mFAILED\033[0m"

#else

#define PASSED "PASSED"
#define FAILED "FAILED"

#endif

#define TEST(ok, name) std::cout << "TEST " << fill(name) << (ok ? PASSED : FAILED) << std::endl
#define TESTE(ok, name, s) TEST(ok, name); if(!get_exception(s).empty()) std::cout <<"\t" << get_exception(s) << std::endl

int __TEST_I;
#define TESTI(ok, name) std::cout << "TEST " << fill(std::to_string(__TEST_I++), 2) <<": " << fill(name) << (ok ? PASSED : FAILED) << std::endl


#define SAMPLE(vars) for(auto [k,v]: vars) std::cout << "\t\t" << k << '\t' << v << std::endl
#define UPDATED(memory, vars) memory.update(vars); SAMPLE(vars)


#endif //ABTM2_TESTING_H
