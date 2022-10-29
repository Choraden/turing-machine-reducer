#include <iostream>
#include <sstream>
#include <cstddef>
#include <cstdlib>
#include "turing_machine.h"

using namespace std;

static bool verbose = true;

static void print_usage(string error) {
    cerr << "ERROR: " << error << "\n"
         << "Usage: tm_interpreter [-q|--quiet] <input_file> <input>\n";
    exit(1);
}

void halt(bool accept) {
    cout << (accept ? "ACCEPT" : "REJECT") << "\n";
    exit(0);
}

vector<vector<string>> tapes;
vector<size_t> heads;
string state = INITIAL_STATE;

void append_blanks_under_heads() {
    for (size_t a = 0; a < tapes.size(); ++a)
        if (heads[a] >= tapes[a].size())
            tapes[a].emplace_back(BLANK);
}

void execute_step(const TuringMachine &tm) {
    pair<string, vector<string>> cur;
    cur.first = state;
    for (size_t a = 0; a < tapes.size(); ++a)
        cur.second.emplace_back(tapes[a][heads[a]]);
    if (tm.transitions.find(cur) == tm.transitions.end()) {
        if (verbose)
            cerr << "No transition from this configuration\n";
        halt(false);
    }
    auto trans = tm.transitions.at(cur);
    state = get<0>(trans);
    for (size_t a = 0; a < tapes.size(); ++a) {
        tapes[a][heads[a]] = get<1>(trans)[a];
        char dir = get<2>(trans)[a];
        if (dir == HEAD_LEFT && !heads[a]) {
            if (verbose)
                cerr << "Head " << a + 1 << " falls off the tape in the next transition\n";
            halt(false);
        }
        heads[a] += dir == HEAD_LEFT ? -1 : dir == HEAD_RIGHT ? 1 : 0;
    }
    append_blanks_under_heads();
}

void print_configuration() {
    cerr << "State: " << state << "\n";
    for (size_t a = 0; a < tapes.size(); ++a) {
        size_t before_head = 0, after_head = 0;
        ostringstream oss;
        oss << "Tape " << (a + 1) << ": ";
        for (size_t b = 0; b < tapes[a].size(); ++b) {
            if (b == heads[a])
                before_head = oss.str().length();
            oss << tapes[a][b];
            if (b == heads[a])
                after_head = oss.str().length();
        }
        cerr << oss.str() << "\n";
        for (size_t b = 0; b < before_head; ++b)
            cerr << " ";
        for (size_t b = before_head; b < after_head; ++b)
            cerr << "^";
        cerr << "\n";
    }
    cerr << "#####################################\n";
}

int main(int argc, char* argv[]) {
    string filename;
    string input;
    int ok = 0;
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--quiet" || arg == "-q")
            verbose = false;
        else {
            if (ok == 0)
                filename = arg;
            else
            if (ok == 1)
                input = arg;
            else
                print_usage("Too many arguments");
            ++ok;
        }
    }
    if (ok != 2)
        print_usage("Not enough arguments");

    FILE *f = fopen(filename.c_str(), "r");
    if (!f) {
        cerr << "ERROR: File " << filename << " does not exist\n";
        return 1;
    }
    TuringMachine tm = read_tm_from_file(f);
    tapes.resize(tm.num_tapes);
    heads.resize(tm.num_tapes);
    tapes[0] = tm.parse_input(input);
    if (tapes[0].empty() && input != "") {
        cerr << "ERROR: The last argument is not a sequence of input letters\n";
        return 1;
    }
    append_blanks_under_heads();

    if (verbose)
        print_configuration();
    for (;;) {
        execute_step(tm);
        if (verbose)
            print_configuration();
        if (state == REJECTING_STATE)
            halt(false);
        if (state == ACCEPTING_STATE)
            halt(true);
    }
}
 
