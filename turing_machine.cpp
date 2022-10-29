#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include "turing_machine.h"

using namespace std;

class Reader {
public:
    bool is_next_token_available() {
        return next_char != '\n' && next_char != EOF;
    }
    
    string next_token() { // only in the current line
        assert(is_next_token_available());
        string res;
        while (next_char != ' ' && next_char != '\t' && next_char != '\n' && next_char != EOF)
            res += get_next_char();
        skip_spaces();
        return res;
    }
    
    void go_to_next_line() { // in particular skips empty lines
        assert(!is_next_token_available());
        while (next_char == '\n') {
            get_next_char();
            skip_spaces();
        }
    }
    
    ~Reader() {
        assert(fclose(input) == 0);
    }
    
    Reader(FILE *input_) : input(input_) {
        assert(input);
        get_next_char();
        skip_spaces();
        if (!is_next_token_available())
            go_to_next_line();
    }
    
    int get_line_num() const {
        return line;
    }

private:
    FILE *input;
    int next_char; // we always have the next char here
    int line = 1;
    
    int get_next_char() {
        if (next_char == '\n')
            ++line;
        int prev = next_char;
        next_char = fgetc(input);
        if (next_char == '#') // skip a comment until EOL or EOF
            while (next_char != '\n' && next_char != EOF)
                next_char = fgetc(input);
        return prev;
    }
    
    void skip_spaces() {
        while (next_char == ' ' || next_char == '\t')
            get_next_char();
    }
};

static bool is_valid_char(int ch) {
    return (ch >= 'a' && ch <= 'z')
        || (ch >= 'A' && ch <= 'Z')
        || (ch >= '0' && ch <= '9')
        || ch == '_' || ch == '-';
}

static bool is_direction(int ch) {
    return ch == HEAD_LEFT || ch == HEAD_RIGHT || ch == HEAD_STAY;
}

// searches for an identifier starting from position pos;
// at the end pos is the position after the identifier
// (if false returned, pos remains unchanged)
static bool check_identifier(string ident, size_t &pos) {
    if (pos >= ident.size())
        return false;
    if (is_valid_char(ident[pos])) {
        ++pos;
        return true;
    }
    if (ident[pos] != '(')
        return false;
    size_t pos2 = pos + 1;
    while (check_identifier(ident, pos2));
    if (pos2 == pos + 1 || pos2 >= ident.size() || ident[pos2] != ')')
        return false;
    pos = pos2 + 1;
    return true;
}

static bool is_identifier(string ident) {
    size_t pos = 0;
    return check_identifier(ident, pos) && pos == ident.length();
}

TuringMachine::TuringMachine(int num_tapes_, vector<string> input_alphabet_, transitions_t transitions_)
    : num_tapes(num_tapes_), input_alphabet(input_alphabet_), transitions(transitions_) {
    assert(num_tapes > 0);
    assert(!input_alphabet.empty());
    for (auto letter : input_alphabet)
        assert(is_identifier(letter) && letter != BLANK);
    for (auto transition : transitions) {
        auto state_before = transition.first.first;
        auto letters_before = transition.first.second;
        auto state_after = get<0>(transition.second);
        auto letters_after = get<1>(transition.second);
        auto directions = get<2>(transition.second);
        assert(is_identifier(state_before) && state_before != ACCEPTING_STATE && state_before != REJECTING_STATE && is_identifier(state_after));
        assert(letters_before.size() == (size_t)num_tapes && letters_after.size() == (size_t)num_tapes && directions.length() == (size_t)num_tapes);
        for (int a = 0; a < num_tapes; ++a)
            assert(is_identifier(letters_before[a]) && is_identifier(letters_after[a]) && is_direction(directions[a]));
    }
}

#define syntax_error(reader, message) \
    for(;;) { \
        cerr << "Syntax error in line " << reader.get_line_num() << ": " << message << "\n"; \
        exit(1); \
    }

static string read_identifier(Reader &reader) {
    if (!reader.is_next_token_available())
        syntax_error(reader, "Identifier expected");
    string ident = reader.next_token();
    size_t pos = 0;
    if (!check_identifier(ident, pos) || pos != ident.length())
        syntax_error(reader, "Invalid identifier \"" << ident << "\"");
    return ident;
}

#define NUM_TAPES "num-tapes:"
#define INPUT_ALPHABET "input-alphabet:"

TuringMachine read_tm_from_file(FILE *input) {
    Reader reader(input);

    // number of tapes
    int num_tapes;
    if (!reader.is_next_token_available() || reader.next_token() != NUM_TAPES)
        syntax_error(reader, "\"" NUM_TAPES "\" expected");
    try {
        if (!reader.is_next_token_available())
            throw 0;
        string num_tapes_str = reader.next_token();
        size_t last;
        num_tapes = stoi(num_tapes_str, &last);
        if (last != num_tapes_str.length() || num_tapes <= 0)
            throw 0;
    } catch (...) {
        syntax_error(reader, "Positive integer expected after \"" NUM_TAPES "\"");
    }
    if (reader.is_next_token_available())
        syntax_error(reader, "Too many tokens in a line");
    reader.go_to_next_line();
    
    // input alphabet
    vector<string> input_alphabet;
    if (!reader.is_next_token_available() || reader.next_token() != INPUT_ALPHABET)
        syntax_error(reader, "\"" INPUT_ALPHABET "\" expected");
    while (reader.is_next_token_available()) {
        input_alphabet.emplace_back(read_identifier(reader));
        if (input_alphabet.back() == BLANK)
            syntax_error(reader, "The blank letter \"" BLANK "\" is not allowed in the input alphabet");
    }
    if (input_alphabet.empty())
        syntax_error(reader, "Identifier expected");
    reader.go_to_next_line();
    
    // transitions
    transitions_t transitions;
    while (reader.is_next_token_available()) {
        string state_before = read_identifier(reader);
        if (state_before == "(accept)" || state_before == "(reject)")
            syntax_error(reader, "No transition can start in the \"" << state_before << "\" state");

        vector<string> letters_before;
        for (int a = 0; a < num_tapes; ++a)
            letters_before.emplace_back(read_identifier(reader));

        if (transitions.find(make_pair(state_before, letters_before)) != transitions.end())
            syntax_error(reader, "The machine is not deterministic");

        string state_after = read_identifier(reader);

        vector<string> letters_after;
        for (int a = 0; a < num_tapes; ++a)
            letters_after.emplace_back(read_identifier(reader));

        string directions;
        for (int a = 0; a < num_tapes; ++a) {
            string dir;
            if (!reader.is_next_token_available() || (dir = reader.next_token()).length() != 1 || !is_direction(dir[0]))
                syntax_error(reader, "Move direction expected, which should be " << HEAD_LEFT << ", " << HEAD_RIGHT << ", or " << HEAD_STAY);
            directions += dir;
        }
        
        if (reader.is_next_token_available()) 
            syntax_error(reader, "Too many tokens in a line");
        reader.go_to_next_line();
        
        transitions[make_pair(state_before, letters_before)] = make_tuple(state_after, letters_after, directions);
    }
    
    return TuringMachine(num_tapes, input_alphabet, transitions);
}

vector<string> TuringMachine::working_alphabet() const {
    set<string> letters(input_alphabet.begin(), input_alphabet.end());
    letters.insert(BLANK);
    for (auto transition : transitions) {
        auto letters_before = transition.first.second;
        auto letters_after = get<1>(transition.second);
        letters.insert(letters_before.begin(), letters_before.end());
        letters.insert(letters_after.begin(), letters_after.end());
    }
    return vector<string>(letters.begin(), letters.end());
}
    
vector<string> TuringMachine::set_of_states() const {
    set<string> states;
    states.insert(INITIAL_STATE);
    states.insert(ACCEPTING_STATE);
    states.insert(REJECTING_STATE);
    for (auto transition : transitions) {
        states.insert(transition.first.first);
        states.insert(get<0>(transition.second));
    }
    return vector<string>(states.begin(), states.end());
}

static void output_vector(ostream &output, vector<string> v) {
   for (string el : v)
        output << " " << el;
}
    
void TuringMachine::save_to_file(ostream &output) const {
    output << NUM_TAPES << " " << num_tapes << "\n"
           << INPUT_ALPHABET;
    output_vector(output, input_alphabet);
    output << "\n";
    for (auto transition : transitions) {
        output << transition.first.first;
        output_vector(output, transition.first.second);
        output << " " << get<0>(transition.second);
        output_vector(output, get<1>(transition.second));
        string directions = get<2>(transition.second);
        for (int a = 0; a < num_tapes; ++a)
            output << " " << directions[a];
        output << "\n";
    }
}

vector<string> TuringMachine::parse_input(std::string input) const {
    set<string> alphabet(input_alphabet.begin(), input_alphabet.end());
    size_t pos = 0;
    vector<string> res;
    while (pos < input.length()) {
        size_t prev_pos = pos;
        if (!check_identifier(input, pos))
            return vector<string>();
        res.emplace_back(input.substr(prev_pos, pos - prev_pos));
        if (alphabet.find(res.back()) == alphabet.end())
            return vector<string>();
    }
    return res;
}
