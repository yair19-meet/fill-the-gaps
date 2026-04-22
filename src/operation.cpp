#include "operation.h"

#include <cstdio>
#include <unistd.h> 

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cmath>
#include <deque>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>

#include <random>
#include <sstream>
#include <stdexcept> 
#include <string>
#include <set>
#include <thread>
#include <unordered_set>

 

#include "generation.h"
#include "trie.h"




Operation::Operation() 
{
}


inline constexpr std::string_view kAlphabet = "abcdefghijklmnopqrstuvwxyz";


std::string Operation::GenerateAWord()
{
    std::uniform_real_distribution<double> distr(0.0, 1.0);

    auto get_random_child = [&](Node* current_node) -> Node* {
        std::vector<char> validLetters;
        for (int j = 0; j < 26; ++j) { // Using 'j' to avoid shadowing
            if (current_node->child(kAlphabet[j]) != nullptr) {
                validLetters.push_back(kAlphabet[j]);
            }
        }
        
        if (validLetters.empty()) {
            return nullptr;
        }
        
        std::uniform_int_distribution<> childDist(0, validLetters.size() - 1);
        char chosenLetter = validLetters[childDist(_gen)];
        return current_node->child(chosenLetter);
    };

  
    std::string word;
    bool found_solution = false;
    while (!found_solution) {
        word = "";
        Node* node = _trie.GetRootRaw();
        float threshold = 1.0f;
        while (node) {
            Node* childNode = get_random_child(node);
            if (!childNode) {
                if (word.length() >= 6 && node->is_word()) {
                    found_solution = true;
                }
                break;
            } else {
                node = childNode;
                word += node->letter();  
            }
            if (node->is_word()) {
                if (word.length() >= 6 && threshold < distr(_gen)) {
                    found_solution = true;
                    break;
                }
            }
            threshold *= 0.85f;
        } 
    } 

    return word;
}


#include <numeric> 
#include <algorithm> 
 
std::vector<std::string> Operation::BreakWord(const std::string& word)
{
    int length = word.length();
    std::vector<std::string> wordParts;

    if (length <= 4) {
        wordParts.push_back(word); 
        return wordParts;
    }

    std::vector<int> cutPoints(length - 1);
    std::iota(cutPoints.begin(), cutPoints.end(), 1); 

    std::shuffle(cutPoints.begin(), cutPoints.end(), _gen);
    cutPoints.resize(4);

    std::sort(cutPoints.begin(), cutPoints.end());
    int s1 = cutPoints[0];
    int s2 = cutPoints[1];
    int s3 = cutPoints[2];
    int s4 = cutPoints[3];

    wordParts.push_back(word.substr(0, s1)); 
    wordParts.push_back(word.substr(s2, s3 - s2));    
    wordParts.push_back(word.substr(s4));             

    return wordParts;
}

std::vector<std::string> Operation::GenerateBrokenWord()
{
    return BreakWord(GenerateAWord());
}


std::pair<bool, std::vector<std::string>> Operation::checkWordValidity(const std::string& userWord, const std::vector<std::string>& brokenWord)
{
    std::vector<std::string> wordsFound;
    Generator<std::pair<Node*, std::string>> gen =  _trie.SearchPreMidSuffixesRoutine(_trie.GetRootRaw(), brokenWord[0], brokenWord[1], brokenWord[2]);
    while (gen.next())
    {
        std::pair<Node*, std::string> values = gen.getValue();
        Node* nodePtr = values.first;
        std::string word = values.second;
        if (nodePtr != nullptr && nodePtr->is_word()) { 
            wordsFound.push_back(word);                            
        }
    }
    if (std::find(wordsFound.begin(), wordsFound.end(), userWord) != wordsFound.end()) {
        return std::make_pair(true, wordsFound);
    }
    return std::make_pair(false, wordsFound);
}
 


void Operation::LoadDictionary(const std::string& filepath) 
{
    std::ifstream file(filepath);

    if (!file.is_open()) {
        std::cerr << "CRITICAL ERROR: Could not open dictionary file: " << filepath << "\n";
        std::cerr << "Make sure 'politics.txt' is in the correct directory!\n";
        return;
    }

    std::string line;
    int wordCount = 0;

    while (std::getline(file, line)) {
        std::string cleanWord = "";

        for (char c : line) {
            if (std::isalpha(c)) {
                cleanWord += std::tolower(c);
            }
        }

        if (cleanWord.length() >= 4) {
            _trie.AddWordToTrie(cleanWord); 
            wordCount++;
        }
    }

    file.close();
    std::cout << "SUCCESS: Loaded " << wordCount << " sanitized words into the Trie.\n";
}





