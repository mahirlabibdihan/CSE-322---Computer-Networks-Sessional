#ifndef __SENDER_H
#define __SENDER_H
#include <iostream>
#include <vector>
#include <bitset>
#include <string>
#include "Color.hpp"
#include "StringArithmetic.hpp"
#include "StringUtil.hpp"
#include "BitMaskUtil.hpp"
using namespace std;
class Sender
{
    string generator;
    int n_bytes, r;

public:
    Sender(string generator, int n_bytes, int r)
    {
        this->generator = generator;
        this->n_bytes = n_bytes;
        this->r = r;
    }

    string send(string message)
    {
        vector<string> data_block_sent = createDataBlock(message);
        vector<string> data_block_sent2 = addCheckBits(data_block_sent);
        string data_word = serializeInColumnMajor(data_block_sent2);
        string code_word = calculateCRC(data_word);
        return code_word;
    }
    vector<string> createDataBlock(string message)
    {
        const int n_rows = message.length() / n_bytes;
        const int n_cols = n_bytes;

        vector<string> sent_data_block(n_rows);

        cout << "Data Block <ascii code of m characters per row>:" << endl;
        for (int i = 0; i < n_rows; i++)
        {
            for (int j = 0; j < n_cols; j++)
            {
                sent_data_block[i] += BitMaskUtil::charToBinary(message[i * n_cols + j]);
            }
            cout << sent_data_block[i] << endl;
        }
        cout << endl;
        return sent_data_block;
    }
    vector<string> addCheckBits(vector<string> sent_data_block)
    {
        const int n_rows = sent_data_block.size();
        const int n_cols = n_bytes * 8 + r;

        for (int i = 0; i < n_rows; i++)
        {
            string new_row(n_cols, '0');

            // Initialize row, with check bits as 0
            for (int j = 0, k = 0; j < n_cols; j++)
            {
                if (!BitMaskUtil::is2Power(j + 1))
                {
                    new_row[j] = sent_data_block[i][k++];
                }
            }
            for (int j = 0; j < r; j++)
            {
                int parity = 0;
                for (int k = j + 1; k < n_cols; k++) // Won't consider the check bit
                {
                    if (BitMaskUtil::isBitSet(k + 1, j))
                    {
                        parity ^= new_row[k] - '0';
                    }
                }

                if (parity) // Not Even parity
                {
                    new_row[(1 << j) - 1] = '1';
                }
            }

            sent_data_block[i] = new_row;
        }

        cout << "Data Block After Adding Check Bits:" << endl;
        for (int i = 0; i < n_rows; i++)
        {
            for (int j = 0; j < n_cols; j++)
            {
                cout << (BitMaskUtil::is2Power(j + 1) ? GREEN : RESET) << sent_data_block[i][j] << RESET;
            }
            cout << endl;
        }
        cout << endl;
        return sent_data_block;
    }

    string serializeInColumnMajor(vector<string> &sent_data_block)
    {
        string data_word;
        const int n_rows = sent_data_block.size();
        const int n_cols = 8 * n_bytes + r;

        for (int i = 0; i < n_cols; i++)
        {
            for (int j = 0; j < n_rows; j++)
            {
                data_word += sent_data_block[j][i];
            }
        }
        cout << "Data-Bits After Column-Wise Serialization:" << endl;
        cout << data_word << endl
             << endl;
        return data_word;
    }

    // For Error Detection
    string calculateCRC(string data_word)
    {
        string msg = data_word, remainder;
        int degree = generator.length() - 1;

        // x^r * msg where generator = x^r + ......
        StringArithmetic::leftShift(msg, degree);
        remainder = StringArithmetic::mod2Division(msg, generator);
        StringUtil::zeroPadding(remainder, degree);

        cout << "Data Bits After Appending CRC Checksum <sent frame>:" << endl;
        cout << data_word << CYAN << remainder << RESET << endl
             << endl;

        return data_word + remainder;
        // new_size = old_size + degree_of_generator
    }
};
#endif