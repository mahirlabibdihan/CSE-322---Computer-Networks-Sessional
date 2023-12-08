#ifndef __STRING_UTIL_H
#define __STRING_UTIL_H
#include <string>
#include <iostream>
using namespace std;
namespace StringUtil
{
    void zeroPadding(string &number, int width)
    {
        while (number.length() < width)
        {
            number = '0' + number;
        }
    }

    string zeroTrimming(string number)
    {
        while (number.length() > 1 && number.front() == '0')
        {
            number.erase(0, 1);
        }
        return number;
    }
};
#endif