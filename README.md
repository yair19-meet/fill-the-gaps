# fill-the-gaps

Fill the gaps is a high-performance word-pattern game that tests your vocabulary and your ability to detect words with hidden gaps.
During the game, the user has two minutes to discover as many words as he can.
An occurrence of a wild card '*' means there exists a gap of zero or more letters.
One sequence of letters with gaps can have multiple solutions - a solution is a word that meets the criteria and exists in the program's dataset.

For instance, possible words that may appear in the game:

    im * ta * n  -> implementation
  
    c * ount * -> country
  
    z * m * a -> zambia

# implementation

The backend of the game is written in Object Oriented C++.
The data structure in which the vocabulary is stored is a Prefix Tree (trie).
The program contains an algorithm that picks an existing random word by traversing the trie randomly, and then randomly creates "break points" for the gaps.
The "broken" word is then displayed to the user.

The "broken" word may match multiple words from the dataset, not only the one the algorithm picked.
After creating the "broken" word, the program creates a list of all possible words in the trie that match the given pattern.
When the user submits his guess, the program checks if the submitted word appears in the list.

While the game engine itself is written in C++, the frontend is written in HTML, CSS and Javascript.
The code is compiled to Web Assembly.


The dataset is extracted from the public github repository dwyl.
