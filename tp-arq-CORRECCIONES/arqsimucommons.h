#ifndef __ARQSIMUCACHE_HPP__
#define __ARQSIMUCACHE_HPP__

#include <stdio.h>
#include <list>
#include <map>
#include <cmath>
#include <sstream>
#include <fstream>
#include <iostream>
#include "pin.H"

int log2(int n);
string uint_to_string(UINT64 n);
string double_to_string(double n);

int log2(int n) {
    return (log10(n))/(log10(2));
}

string uint_to_string(UINT64 n) {
    std::stringstream stream;
    stream << n;
    return stream.str();
}

string double_to_string(double n) {
    std::stringstream stream;
    stream << n;
    return stream.str();
}

#endif
