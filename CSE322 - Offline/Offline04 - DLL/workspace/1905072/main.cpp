#include <iostream>
#include <string>
#include <random>
#include <chrono>
#include "Sender.hpp"
#include "Receiver.hpp"
using namespace std;

string message;
int n_bytes;
double p_toggle;
string generator;
int r;

int getNCheckBits(int k)
{
    // Linear search
    for (int r = 0;; r++)
    {
        // 1 + k + r <= 2^r
        if (1 + k + r <= (1 << r))
        {
            return r;
        }
    }
    return -1; // Unreachable
}

void config()
{
    // freopen("input/test3.txt", "r", stdin);

    cout << "Enter Data String: ";
    getline(cin, message);

    cout << "Enter Number of Data Bytes in a Row <m>: ";
    cin >> n_bytes;

    cout << "Enter Probability <p>: ";
    cin >> p_toggle;

    cout << "Enter Generator Polynomial: ";
    cin >> generator;

    while (message.length() % n_bytes)
    {
        message += '~';
    }
    cout << endl
         << endl;
    cout << "Data String After Padding: ";
    cout << message << endl
         << endl;

    r = getNCheckBits(8 * n_bytes);
}

vector<bool> toggleBits(string &frame)
{
    int len = frame.length();

    vector<bool> mark(len);
    // unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    // default_random_engine gen(1905072);
    // uniform_real_distribution<double> dist(0.0, 1.0);
    mt19937 gen(1905072);
    bernoulli_distribution dist(p_toggle);
    for (int i = 0; i < len; i++)
    {
        if (dist(gen))
        {
            frame[i] = StringArithmetic::charXor(frame[i], '1');
            mark[i] = true;
        }
        else
        {
            mark[i] = false;
        }
    }

    return mark;
}

void simulation()
{
    // Send
    Sender sender(generator, n_bytes, r);
    string frame = sender.send(message);

    // Physical Medium
    vector<bool> mark = toggleBits(frame);

    // Receive
    Receiver receiver(generator, n_bytes, r);
    receiver.setMarks(mark);
    receiver.receive(frame);
}
int main()
{
    config();
    simulation();
}