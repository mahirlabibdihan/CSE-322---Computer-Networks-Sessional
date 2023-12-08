#ifndef __BIT_MASK_UTIL_H
#define __BIT_MASK_UTIL_H
#include <string>
#include <bitset>
using namespace std;
namespace BitMaskUtil
{
    bool is2Power(int number)
    {
        return !(number & (number - 1));
    }
    bool isBitSet(int number, int idx)
    {
        return (number & (1ULL << idx)) != 0;
    }
    void setBit(int &number, int idx)
    {
        number |= (1ULL << idx);
    }
    string charToBinary(char x)
    {
        return bitset<8>(x).to_string();
    }
};
#endif