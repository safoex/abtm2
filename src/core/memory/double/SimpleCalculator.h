//
// Created by safoex on 04.09.19.
//

#ifndef ABTM2_SIMPLECALCULATOR_H
#define ABTM2_SIMPLECALCULATOR_H

#include <core/common.h>

namespace calc {
    bool delim(char c) {
        return c == ' ';
    }

    bool is_op(char c) {
        static std::set<char> ops{'+', '-', '*', '/', '>', '<', '=', '!', '&', '|', '!'};
        return ops.count(c) > 0;
    }

    int priority(char op) {
        if (op < 0)
            return 6; // op == -'+' || op == -'-'
        return
                op == '&' || op == '|' ? 1 :
                op == '<' || op == '>' || op == '=' || op == '!' ? 2 :
                op == '+' || op == '-' ? 3 :
                op == '*' || op == '/' || op == '%' ? 4 :
                -1;
    }

    void process_op(std::vector<double> &st, char op) {
        if (op < 0) {
            double l = st.back();
            st.pop_back();
            switch (-op) {
                case '+':
                    st.push_back(l);
                    break;
                case '-':
                    st.push_back(-l);
                    break;
                default:
                    st.push_back(l);
                    break;
            }
        } else {
            double r = st.back();
            st.pop_back();
            double l = st.back();
            st.pop_back();
            switch (op) {
                case '+':
                    st.push_back(l + r);
                    break;
                case '-':
                    st.push_back(l - r);
                    break;
                case '*':
                    st.push_back(l * r);
                    break;
                case '/':
                    st.push_back(l / r);
                    break;
                case '>':
                    st.push_back(l > r);
                    break;
                    //case '>=':  st.push_back (l >= r);  break;
                case '<':
                    st.push_back(l < r);
                    break;
                    //case '<=':  st.push_back (l <= r);  break;
                case '=':
                    st.push_back(l == r);
                    break;
                case '!':
                    st.push_back(l != r);
                    break;
                case '&':
                    st.push_back(bool(l) && bool(r));
                    break;
                case '|':
                    st.push_back(bool(l) || bool(r));
                    break;
                default:
                    throw std::runtime_error("Error: wrong operator" + std::string() + op + " in calc"); //TODO
            }
        }
    }

    bool isunary(char op) {
        return op == '+' || op == '-';
    }

    bool isalnum_(char c) {
        return isalnum(c) || c == '_';
    }

    bool isdigit_or_dot(char c) {
        return c == '.' || isdigit(c);
    }

    double calc(std::string const &s, const abtm::dictOf<double> &m) {
        bool may_unary = false;
        std::vector<double> st;
        std::vector<char> op;
        for (size_t i = 0; i < s.length(); ++i)
            if (!delim(s[i])) {
                if (s[i] == '(') {
                    op.push_back('(');
                    may_unary = true;
                } else if (s[i] == ')') {
                    while (op.back() != '(')
                        process_op(st, op.back()), op.pop_back();
                    op.pop_back();
                    may_unary = false;
                } else if (is_op(s[i])) {
                    char curop = s[i];
                    if (may_unary && isunary(curop)) curop = -curop;
                    while (!op.empty() && (
                            (curop >= 0 && priority(op.back()) >= priority(curop)) ||
                            (curop < 0 && priority(op.back()) > priority(curop)))
                            )
                        process_op(st, op.back()), op.pop_back();
                    op.push_back(curop);
                    may_unary = true;
                } else {
                    std::string operand;
                    while (i < s.length() && (isalnum_(s[i]) || isdigit_or_dot(s[i])))
                        operand += s[i++];
                    --i;
                    if (isdigit_or_dot(operand[0]))
                        st.push_back(strtod(operand.c_str(), nullptr));
                    else
                        st.push_back(m.at(operand));
                    may_unary = false;
                }
            }
        while (!op.empty())
            process_op(st, op.back()), op.pop_back();
        return st.back();
    }

    abtm::keys find_vars(std::string const &s) {
        abtm::keys vars{};
        bool may_unary = false;
        std::vector<double> st;
        std::vector<char> op;
        for (size_t i = 0; i < s.length(); ++i)
            if (!delim(s[i])) {
                if (s[i] == '(') {
                    op.push_back('(');
                    may_unary = true;
                } else if (s[i] == ')') {
                    while (op.back() != '(')
                        process_op(st, op.back()), op.pop_back();
                    op.pop_back();
                    may_unary = false;
                } else if (is_op(s[i])) {
                    char curop = s[i];
                    if (may_unary && isunary(curop)) curop = -curop;
                    while (!op.empty() && (
                            (curop >= 0 && priority(op.back()) >= priority(curop)) ||
                            (curop < 0 && priority(op.back()) > priority(curop)))
                            )
                        process_op(st, op.back()), op.pop_back();
                    op.push_back(curop);
                    may_unary = true;
                } else {
                    std::string operand;
                    while (i < s.length() && (isalnum_(s[i]) || isdigit_or_dot(s[i])))
                        operand += s[i++];
                    --i;
                    if (isdigit_or_dot(operand[0]))
                        st.push_back(strtod(operand.c_str(), nullptr));
                    else {
                        st.push_back(0);
                        vars.insert(operand);
                    }
                    may_unary = false;
                }
            }
        return vars;
    }
}

#endif //ABTM2_SIMPLECALCULATOR_H
