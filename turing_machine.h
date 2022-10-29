#ifndef __TURING_MACHINE_H
#define __TURING_MACHINE_H

#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

// an identifier (which can be used as a name of a letter or a state) is of the form:
// * a single letter from {A-Z, a-z, 0-9, _, -}
// * a nonempty sequence of indentifiers surrounded in brackets (...)
// examples of valid identifiers: A, 0, _, (0), (start), ((abc)-(def)(_))

// special identifiers:
#define BLANK "_"
#define INITIAL_STATE "(start)"
#define ACCEPTING_STATE "(accept)"
#define REJECTING_STATE "(reject)"

// in which direction head moves:
#define HEAD_LEFT '<'
#define HEAD_RIGHT '>'
#define HEAD_STAY '-'

typedef std::map<std::pair<std::string, std::vector<std::string>>, std::tuple<std::string, std::vector<std::string>, std::string>> transitions_t;

struct TuringMachine {
    int num_tapes;
    
    std::vector<std::string> input_alphabet;
    
    transitions_t transitions;
    // (state, [letter_on_tape_1, ..., letter_on_tape_k])
    //    -> (new_state, [new_letter_on_tape_1, ..., new_letter_on_tape_k], [move_on_tape_1, ..., move_on_tape_k])
    
    TuringMachine(int, std::vector<std::string>, transitions_t);

    std::vector<std::string> working_alphabet() const;
    
    std::vector<std::string> set_of_states() const;
    
    void save_to_file(std::ostream &output) const;
    
    std::vector<std::string> parse_input(std::string input) const;
    // ERROR <=> input!="" && returned_value.empty()
};

static inline std::ostream &operator<<(std::ostream &output, const TuringMachine &tm) {
    tm.save_to_file(output);
    return output;
}

TuringMachine read_tm_from_file(FILE *input);

#endif
