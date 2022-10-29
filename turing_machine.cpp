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

int NUMBER_OF_PARENTHESES = 0;

void set_number_of_parentheses(vector<string> working_alphabet) {
    for (auto letter: working_alphabet) {
        int paren = 0;
        for (auto c: letter) {
            if (c == '(') {
                paren++;
            }
        }
        if (paren > NUMBER_OF_PARENTHESES) {
            NUMBER_OF_PARENTHESES = paren;
        }
    }
}

// Makes letter with token which means that the logical head is above this letter.
string make_logical_head(string letter) {
    string res;
    for (int i = 0; i <= NUMBER_OF_PARENTHESES; i++) {
        res += "(";
    }
    res += letter + "-H";
    for (int i = 0; i <= NUMBER_OF_PARENTHESES; i++) {
        res += ")";
    }
    return res;
}

// Makes in transition for one tape machine.
pair<string, vector<string>> make_in(string state, string letter) {
    return make_pair(state, vector<string>{letter});
}

// Makes out transition for one tape machine.
tuple<string, vector<string>, string> make_out(string state, string letter, char dir) {
    return make_tuple(state, vector<string>{letter}, string{dir});
}

// Makes state that comes from original state provided by user.
string make_user_state(string original_state, string move, int tape) {
    return "(U-" + original_state + "-" + move + "-" + to_string(tape) + ")";
}

const string INIT_FIND_SECOND_TAPE = "(init-findSecondTape)";
const string INIT_PUT_SECOND_HEAD = "(init-putSecondHead)";
const string INIT_PUT_END_OF_SECOND_HEAD = "(init-putEndOfSecondTape)";
const string INIT_BACK_TO_BORDER = "(init-backToBorder)";
const string INIT_BACK_TO_FRONT = "(init-backToFront)";

string TAPE_BORDER;

void set_tape_border() {
    string res;
    for (int i = 0; i <= NUMBER_OF_PARENTHESES; i++) {
        res += "(";
    }
    res += "tape-border";
    for (int i = 0; i <= NUMBER_OF_PARENTHESES; i++) {
        res += ")";
    }

    TAPE_BORDER = res;
}

string TAPE_END;

void set_tape_end() {
    string res;
    for (int i = 0; i <= NUMBER_OF_PARENTHESES; i++) {
        res += "(";
    }
    res += "tape-end";
    for (int i = 0; i <= NUMBER_OF_PARENTHESES; i++) {
        res += ")";
    }

    TAPE_END = res;
}


const string MOVE_HEAD_RIGHT = "headRight";
const string MOVE_HEAD_LEFT = "headLeft";


const string TO_SECOND_TAPE = "toSecondTape";

const string EXT_TAPE_TAPE_BORDER = "extendTape-tapeBorder";
const string EXT_TAPE_TAPE_END = "extendTape-tapeEnd";
const string EXT_TAPE_MOVE_BACK = "extendTape-moveBack";

string extend_tape_with_letter(string letter) {
    return "extendTape-" + letter;
}

string extend_tape_back_to_first_tape(string letter) {
    return "backToFirstTape-" + letter;
}

// Produce states that will split tape into two and place logical heads.
transitions_t make_init_states(vector<string> &input_alphabet) {
    transitions_t res;

    // Put logical head at the front of the tape and start finding second tape.
    for (auto letter: input_alphabet) {
        res[make_in(INITIAL_STATE, letter)] = make_out(INIT_FIND_SECOND_TAPE, make_logical_head(letter), HEAD_RIGHT);
    }
    res[make_in(INITIAL_STATE, BLANK)] = make_out(INIT_FIND_SECOND_TAPE, make_logical_head(BLANK), HEAD_RIGHT);


    // Find second tape.
    for (auto letter: input_alphabet) {
        res[make_in(INIT_FIND_SECOND_TAPE, letter)] = make_out(INIT_FIND_SECOND_TAPE, letter, HEAD_RIGHT);
    }

    // Put tape border, move right.
    res[make_in(INIT_FIND_SECOND_TAPE, BLANK)] = make_out(INIT_PUT_SECOND_HEAD, TAPE_BORDER, HEAD_RIGHT);
    // Put logic head at the second tape, move right.
    res[make_in(INIT_PUT_SECOND_HEAD, BLANK)] = make_out(INIT_PUT_END_OF_SECOND_HEAD, make_logical_head(BLANK), HEAD_RIGHT);
    // Put tape end.
    res[make_in(INIT_PUT_END_OF_SECOND_HEAD, BLANK)] = make_out(INIT_BACK_TO_BORDER, TAPE_END, HEAD_LEFT);

    // Back to front of the tape.
    res[make_in(INIT_BACK_TO_BORDER, make_logical_head(BLANK))] = make_out(INIT_BACK_TO_BORDER, make_logical_head(BLANK), HEAD_LEFT);
    res[make_in(INIT_BACK_TO_BORDER, TAPE_BORDER)] = make_out(INIT_BACK_TO_FRONT, TAPE_BORDER, HEAD_LEFT);
    for (auto letter: input_alphabet) {
        res[make_in(INIT_BACK_TO_FRONT, letter)] = make_out(INIT_BACK_TO_FRONT, letter, HEAD_LEFT);
    }

    return res;
}

transitions_t make_one_tape_transitions_from_two(std::pair<std::string, std::vector<std::string>> in, std::tuple<std::string, std::vector<std::string>, std::string> out, vector<string> alphabet) {
    auto letter_in_tape_1 = in.second[0];
    auto letter_in_tape_2 = in.second[1];
    auto letter_out_tape_1 = get<1>(out)[0];
    auto letter_out_tape_2 = get<1>(out)[1];
    auto dir_tape_1 = get<2>(out)[0];
    auto dir_tape_2 = get<2>(out)[1];

    // Save information about input and output letters in state.
    auto state_in = in.first + "-(" + letter_in_tape_1 + ")-(" + letter_in_tape_2 + ")";

    transitions_t res;

    // Add transition from internal initial state to user-defined start state.
    if (in.first == INITIAL_STATE) {
        // Second tape is always empty on the start.
        if (letter_in_tape_2 == BLANK) {
            res[make_in(INIT_BACK_TO_FRONT, make_logical_head(letter_in_tape_1))] = make_out(make_user_state(state_in, "", 1), make_logical_head(letter_in_tape_1), HEAD_STAY);
        }
    }

    if (dir_tape_1 == HEAD_LEFT) {
        res[make_in(make_user_state(state_in, "", 1), make_logical_head(letter_in_tape_1))] = make_out(make_user_state(state_in, MOVE_HEAD_LEFT, 1), letter_out_tape_1, HEAD_LEFT);
        for (auto letter: alphabet) {
            res[make_in(make_user_state(state_in, MOVE_HEAD_LEFT, 1), letter)] = make_out(make_user_state(state_in, TO_SECOND_TAPE, 1), make_logical_head(letter), HEAD_RIGHT);
        }
    }

    if (dir_tape_1 == HEAD_RIGHT) {
        res[make_in(make_user_state(state_in, "", 1), make_logical_head(letter_in_tape_1))] = make_out(make_user_state(state_in, MOVE_HEAD_RIGHT, 1), letter_out_tape_1, HEAD_RIGHT);
        for (auto letter: alphabet) {
            res[make_in(make_user_state(state_in, MOVE_HEAD_RIGHT, 1), letter)] = make_out(make_user_state(state_in, TO_SECOND_TAPE, 1), make_logical_head(letter), HEAD_RIGHT);
        }

        // Extend first tape if necessary.
        res[make_in(make_user_state(state_in, MOVE_HEAD_RIGHT, 1), TAPE_BORDER)] = make_out(make_user_state(state_in, EXT_TAPE_TAPE_BORDER, 1), BLANK, HEAD_RIGHT);
        for (auto letter1: alphabet) {
            res[make_in(make_user_state(state_in, EXT_TAPE_TAPE_BORDER, 1), letter1)] = make_out(make_user_state(state_in, extend_tape_with_letter(letter1), 1), TAPE_BORDER, HEAD_RIGHT);
            res[make_in(make_user_state(state_in, EXT_TAPE_TAPE_BORDER, 1), make_logical_head(letter1))] = make_out(make_user_state(state_in, extend_tape_with_letter(make_logical_head(letter1)), 1), TAPE_BORDER, HEAD_RIGHT);
            for (auto letter2: alphabet) {
                res[make_in(make_user_state(state_in, extend_tape_with_letter(letter1), 1), letter2)] = make_out(make_user_state(state_in, extend_tape_with_letter(letter2), 1), letter1, HEAD_RIGHT);
                res[make_in(make_user_state(state_in, extend_tape_with_letter(letter1), 1), make_logical_head(letter2))] = make_out(make_user_state(state_in, extend_tape_with_letter(make_logical_head(letter2)), 1), letter1, HEAD_RIGHT);
                res[make_in(make_user_state(state_in, extend_tape_with_letter(make_logical_head(letter1)), 1), letter2)] = make_out(make_user_state(state_in, extend_tape_with_letter(letter2), 1), make_logical_head(letter1), HEAD_RIGHT);
            }
            res[make_in(make_user_state(state_in, extend_tape_with_letter(letter1), 1), TAPE_END)] = make_out(make_user_state(state_in, EXT_TAPE_TAPE_END, 1), letter1, HEAD_RIGHT);
            res[make_in(make_user_state(state_in, extend_tape_with_letter(make_logical_head(letter1)), 1), TAPE_END)] = make_out(make_user_state(state_in, EXT_TAPE_TAPE_END, 1), make_logical_head(letter1), HEAD_RIGHT);
        }

        // Back to moving logical head to the right.
        res[make_in(make_user_state(state_in, EXT_TAPE_TAPE_END, 1), BLANK)] = make_out(make_user_state(state_in, EXT_TAPE_MOVE_BACK, 1), TAPE_END, HEAD_LEFT);
        for (auto letter: alphabet) {
            res[make_in(make_user_state(state_in, EXT_TAPE_MOVE_BACK, 1), letter)] = make_out(make_user_state(state_in, EXT_TAPE_MOVE_BACK, 1), letter, HEAD_LEFT);
            res[make_in(make_user_state(state_in, EXT_TAPE_MOVE_BACK, 1), make_logical_head(letter))] = make_out(make_user_state(state_in, EXT_TAPE_MOVE_BACK, 1), make_logical_head(letter), HEAD_LEFT);
        }
        res[make_in(make_user_state(state_in, EXT_TAPE_MOVE_BACK, 1), TAPE_BORDER)] = make_out(make_user_state(state_in, MOVE_HEAD_RIGHT, 1), TAPE_BORDER, HEAD_LEFT);
    }

    if (dir_tape_1 == HEAD_STAY) {
        res[make_in(make_user_state(state_in, "", 1), make_logical_head(letter_in_tape_1))] = make_out(make_user_state(state_in, TO_SECOND_TAPE, 1), make_logical_head(letter_out_tape_1), HEAD_RIGHT);
    }

    // Move physical head to the logical head on the second tape.
    for (auto letter: alphabet) {
        res[make_in(make_user_state(state_in, TO_SECOND_TAPE, 1), letter)] = make_out(make_user_state(state_in, TO_SECOND_TAPE, 1), letter, HEAD_RIGHT);
        res[make_in(make_user_state(state_in, TO_SECOND_TAPE, 1), TAPE_BORDER)] = make_out(make_user_state(state_in, TO_SECOND_TAPE, 2), TAPE_BORDER, HEAD_RIGHT);
        res[make_in(make_user_state(state_in, TO_SECOND_TAPE, 2), letter)] = make_out(make_user_state(state_in, TO_SECOND_TAPE, 2), letter, HEAD_RIGHT);
    }

    if (dir_tape_2 == HEAD_LEFT) {
        res[make_in(make_user_state(state_in, TO_SECOND_TAPE, 2), make_logical_head(letter_in_tape_2))] = make_out(make_user_state(state_in, MOVE_HEAD_LEFT, 2), letter_out_tape_2, HEAD_LEFT);
        for (auto letter: alphabet) {
            res[make_in(make_user_state(state_in, MOVE_HEAD_LEFT, 2), letter)] = make_out(make_user_state(state_in, extend_tape_back_to_first_tape(letter), 2), make_logical_head(letter), HEAD_LEFT);
        }
    }

    if (dir_tape_2 == HEAD_RIGHT) {
        res[make_in(make_user_state(state_in, TO_SECOND_TAPE, 2), make_logical_head(letter_in_tape_2))] = make_out(make_user_state(state_in, MOVE_HEAD_RIGHT, 2), letter_out_tape_2, HEAD_RIGHT);
        for (auto letter: alphabet) {
            res[make_in(make_user_state(state_in, MOVE_HEAD_RIGHT, 2), letter)] = make_out(make_user_state(state_in, extend_tape_back_to_first_tape(letter), 2), make_logical_head(letter), HEAD_LEFT);
        }

        // Extend second tape if necessary.
        res[make_in(make_user_state(state_in, MOVE_HEAD_RIGHT, 2), TAPE_END)] = make_out(make_user_state(state_in, EXT_TAPE_TAPE_END, 2), BLANK, HEAD_RIGHT);
        // Back to moving logical head to the right.
        res[make_in(make_user_state(state_in, EXT_TAPE_TAPE_END, 2), BLANK)] = make_out(make_user_state(state_in, MOVE_HEAD_RIGHT, 2), TAPE_END, HEAD_LEFT);
    }

    if (dir_tape_2 == HEAD_STAY) {
        res[make_in(make_user_state(state_in, TO_SECOND_TAPE, 2), make_logical_head(letter_in_tape_2))] = make_out(make_user_state(state_in, extend_tape_back_to_first_tape(letter_out_tape_2), 2), make_logical_head(letter_out_tape_2), HEAD_LEFT);
    }

    // Move physical head back to the logical head on the first tape.
    for (auto letter1: alphabet) {
        for (auto letter2: alphabet) {
            res[make_in(make_user_state(state_in, extend_tape_back_to_first_tape(letter1), 2), letter2)] = make_out(make_user_state(state_in, extend_tape_back_to_first_tape(letter1), 2), letter2, HEAD_LEFT);
            res[make_in(make_user_state(state_in, extend_tape_back_to_first_tape(letter1), 2), TAPE_BORDER)] = make_out(make_user_state(state_in, extend_tape_back_to_first_tape(letter1), 1), TAPE_BORDER, HEAD_LEFT);
            res[make_in(make_user_state(state_in, extend_tape_back_to_first_tape(letter1), 1), letter2)] = make_out(make_user_state(state_in, extend_tape_back_to_first_tape(letter1), 1), letter2, HEAD_LEFT);
        }
    }

    // Add Out state, which connects different states.
    for (auto letter1: alphabet) {
        for (auto letter2: alphabet) {
            // Notice that it is not necessary to check for REJECTING_STATE here, as there are simply no transitions starting from REJECT_STATE, so it will be automatically rejected.
            if (get<0>(out) == ACCEPTING_STATE) {
                res[make_in(make_user_state(state_in, extend_tape_back_to_first_tape(letter1), 1), make_logical_head(letter2))] = make_out(ACCEPTING_STATE, make_logical_head(letter2), HEAD_STAY);
            } else {
                auto state_out = get<0>(out) + "-(" + letter2 + ")-(" + letter1 + ")";
                res[make_in(make_user_state(state_in, extend_tape_back_to_first_tape(letter1), 1), make_logical_head(letter2))] = make_out(make_user_state(state_out, "", 1), make_logical_head(letter2), HEAD_STAY);
            }
        }
    }

    return res;
}

TuringMachine TuringMachine::reduce_two_tapes_to_one() {
    assert(num_tapes == 2 && "Number of tapes different from 2");
    set_number_of_parentheses(working_alphabet());
    set_tape_border();
    set_tape_end();

    transitions_t new_transitions;

    auto init_states = make_init_states(input_alphabet);
    new_transitions.insert(init_states.begin(), init_states.end());


    for (auto transition: transitions) {
        auto nt = make_one_tape_transitions_from_two(transition.first, transition.second, working_alphabet());
        new_transitions.insert(nt.begin(), nt.end());
    }

    return TuringMachine(1, input_alphabet, new_transitions);
}
