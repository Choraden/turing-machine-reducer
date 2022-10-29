#include <iostream>
#include <sstream>
#include <fstream>
#include "turing_machine.h"

using namespace std;

static void print_usage(string error) {
    cerr << "ERROR: " << error << "\n"
         << "Usage: tm_reducer <two tape machine file> <where to save one tape machine>\n";
    exit(1);
}


int main(int argc, char *argv[]) {
    string filename;

    int ok = 0;
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];

        if (ok == 0)
            filename = arg;
        else if (ok == 1) {}
        else
            print_usage("Too many arguments");
        ++ok;
    }

    if (ok != 2)
        print_usage("Not enough arguments");

    FILE *f = fopen(filename.c_str(), "r");
    if (!f) {
        cerr << "ERROR: File " << filename << " does not exist\n";
        return 1;
    }

    TuringMachine tm = read_tm_from_file(f);
    auto reduced_tm = tm.reduce_two_tapes_to_one();
    
    ofstream f_out(argv[2]);
    reduced_tm.save_to_file(f_out);
}
