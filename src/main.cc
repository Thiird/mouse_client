#include <iostream>
#include <vector>
#include <string>

#include "./include/com_port.hpp"

using namespace std;

int main()
{
    int err = 0;

    err = init_com_port();
    if (!err)
    {
        cout << "ERR" << endl;
    }
}
