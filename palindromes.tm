
# Example 2-tape Turing machine, recognizing the language of palindromes over {a,b}

# Text after "#" is a comment
# For information on possible names of states and letters check a comment in turing_machine.h

num-tapes: 2          # mendatory - number of tapes
input-alphabet: a b   # mendatory - letters allowed to occur on input

# next, we have a list of transitions, in the format:
# <p> <a_1> ... <a_k> <q> <b_1> ... <b_k> <d_1> ... <d_k>
# where:
# * p and q are states before / after the transition
# * a_1, ..., a_k are letters under the head on tapes 1,...,k before the transition
# * b_1, ..., b_k - likewise, but after the transition
# * d_1, ..., d_k - directions in which each head is moved; can be <, >, -

# first, we copy the input to the second tape, but we shift it one cell right
(start) a _ (copyA) _ _ > >
(start) b _ (copyB) _ _ > >
(copyA) a _ (copyA) a a > >
(copyA) b _ (copyB) a a > >
(copyB) a _ (copyA) b b > >
(copyB) b _ (copyB) b b > >
(copyA) _ _ (back) a a < >    # start going back
(copyB) _ _ (back) b b < >

# we go back on the first tape
(back) a _ (back) a _ < -
(back) b _ (back) b _ < -
(back) _ _ (check) _ _ > <    # start checking

# we read the first tape left-to-right; the second tape right-to-left
(check) a a (check) a a > <
(check) b b (check) b b > <
(check) _ _ (accept) _ _ - -  # everything OK

(start) _ _ (accept) _ _ - -  # special case: the empty word should also be accepted
