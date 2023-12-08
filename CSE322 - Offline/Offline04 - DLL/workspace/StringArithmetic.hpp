#ifndef __STRING_ARITHMETIC_H
#define __STRING_ARITHMETIC_H
#include <string>
#include <iostream>
using namespace std;

namespace StringArithmetic
{
    char charXor(char a, char b)
    {
        return ((a - '0') ^ (b - '0')) + '0';
    }

    void leftShift(string &number, int shift)
    {
        for (int i = 0; i < shift; i++)
        {
            number += '0';
        }
    }

    string mod2Division(string dividend, string divisor)
    {
        const int dividend_len = dividend.length();
        const int divisor_len = divisor.length();

        if (divisor_len > dividend_len)
        {
            return dividend;
        }

        string remainder(dividend.begin(), dividend.begin() + divisor_len - 1);
        string remaining_dividend(dividend.begin() + divisor_len - 1, dividend.end());
        string new_dividend;

        while (!remaining_dividend.empty())
        {
            new_dividend = remainder + remaining_dividend.front();

            remaining_dividend.erase(0, 1);

            string new_divisor(divisor_len, '0');
            if (new_dividend.front() == '1')
            {
                new_divisor = divisor;
            }
            // If the leftmost bit of the dividend (or the part used in each step) is 0, the step cannot use the regular divisor; we need to use an all - 0s divisor.

            // step
            remainder = "";
            // xor of two strings skipping 1st char
            for (int i = 1; i < divisor_len; i++)
            {
                remainder += charXor(new_dividend[i], new_divisor[i]);
            }
        }
        return remainder.empty() ? "0" : remainder;
    }
};
#endif