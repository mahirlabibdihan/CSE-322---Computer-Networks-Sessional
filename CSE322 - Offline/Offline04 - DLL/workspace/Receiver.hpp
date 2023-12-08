#ifndef __RECEIVER_H
#define __RECEIVER_H
#include <string>
#include <iostream>
#include <vector>
#include "Color.hpp"
#include "StringArithmetic.hpp"
#include "BitMaskUtil.hpp"
using namespace std;

class Receiver
{
    string generator;
    vector<bool> mark;
    int n_bytes, r;

public:
    Receiver(string generator, int n_bytes, int r)
    {
        this->generator = generator;
        this->n_bytes = n_bytes;
        this->r = r;
    }
    void receive(string received_frame)
    {
        printFrame(received_frame);
        verifyFrame(received_frame);
        vector<string> received_data_block = removeCRC(received_frame);
        vector<string> received_data_block2 = removeCheckBits(received_data_block);
        string message = decodeData(received_data_block2);
    }

    void printFrame(string frame)
    {
        int len = frame.size();

        cout << "Received Frame:" << endl;
        for (int i = 0; i < len; i++)
        {
            cout << (mark[i] ? RED : RESET) << frame[i] << RESET;
        }

        cout << endl
             << endl;
    }
    void setMarks(vector<bool> mark)
    {
        this->mark = mark;
    }
    void verifyFrame(string received_frame)
    {
        string remainder = StringUtil::zeroTrimming(StringArithmetic::mod2Division(received_frame, generator));
        cout << "Result of CRC Checksum Matching: " << (remainder == "0" ? "No Error Detected" : "Error Detected") << endl
             << endl;
    }

    vector<string> removeCRC(string received_frame)
    {
        int degree = generator.length() - 1;
        received_frame.substr(0, received_frame.length() - degree);

        int len = received_frame.length();

        const int n_cols = 8 * n_bytes + r;
        const int n_rows = len / n_cols;

        vector<string> received_data_block(n_rows);

        for (int i = 0, k = 0; i < len; i++, k %= n_rows)
        {
            received_data_block[k++] += received_frame[i];
        } 

        cout << "Data Block After Removing CRC Checksum Bits: " << endl;
        for (int i = 0; i < n_rows; i++)
        {
            for (int j = 0; j < n_cols; j++)
            {
                cout << (mark[i + n_rows * j] ? RED : RESET) << received_data_block[i][j] << RESET;
            }
            cout << endl;
        }
        cout << endl;
        return received_data_block;
    }

    vector<string> removeCheckBits(vector<string> received_data_block)
    {
        const int n_rows = received_data_block.size();
        const int n_cols = 8 * n_bytes + r;

        for (int i = 0; i < n_rows; i++)
        {
            int sum = 0;

            // Iterate over each parity bit
            for (int j = 0; j < r; j++)
            {
                // pick j-th check bit
                int parity = 0;
                for (int k = j; k < n_cols; k++) // Consider the check bit
                {
                    if (BitMaskUtil::isBitSet(k + 1, j)) // under consideration
                    {
                        parity ^= received_data_block[i][k] - '0';
                    }
                }
                if (parity) // Not even parity
                {
                    BitMaskUtil::setBit(sum, j);
                }
            }

            if (sum > 0 && sum <= n_cols) // error detected
            {
                received_data_block[i][sum - 1] = StringArithmetic::charXor(received_data_block[i][sum - 1], '1'); // error corrected
            }

            string new_row;
            for (int j = 0; j < n_cols; j++)
            {
                if (!BitMaskUtil::is2Power(j + 1)) // skip check bits
                {
                    new_row += received_data_block[i][j];
                }
            }
            received_data_block[i] = new_row;
        }

        cout << "Data Block After Removing Check Bits:" << endl;
        for (string data_row : received_data_block)
        {
            cout << data_row << endl;
        }
        cout << endl;

        return received_data_block;
    }

    string decodeData(vector<string> received_data_block)
    {
        const int n_rows = received_data_block.size();
        string received;
        for (int i = 0; i < n_rows; i++)
        {
            for (int j = 0; j < n_bytes; j++)
            {
                string byte;
                for (int k = 0; k < 8; k++)
                {
                    byte += received_data_block[i][j * 8 + k];
                }
                received += (char)toAscii(byte);
            }
        }
        cout << "Output Frame: " << received << endl;
        return received;
    }

    int toAscii(string s)
    {
        return stoi(s, 0, 2);
    }
};
#endif