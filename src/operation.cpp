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

#include <arpa/inet.h> 

#include "generation.h"
#include "trie.h"




const std::filesystem::path Operation::_files_data_file = word_finder::getDataDir() / "files_data.txt";


Operation::Operation() 
{
    _gen = std::random_device{}();
}

Operation::~Operation()
{
}


#include <random>
inline constexpr std::string_view kAlphabet = "abcdefghijklmnopqrstuvwxyz";

std::vector<std::string> Operation::GenerateBrokenWord()
{

    // distributions for choosing interval
    std::uniform_int_distribution<> distrFirstInterval(0, 3); 
    std::uniform_int_distribution<> distrNormalInterval(1, 4);

    // here we save the "broken" word as parts seperated by implicit wild cards.
    std::vector<std::string> wordParts;

    // we ran the algorithm until we get a valid solution
    bool found_solution = false;


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


    while (!found_solution) {
        wordParts = {};
        bool reached_dead_end = false;
        int num_of_stars = 0;
        Node* node = _trie.GetRootRaw();

        bool isFirstInterval = true;

        while (!reached_dead_end) {
            std::string currentPart = "";
            int intervalLength;
            if (isFirstInterval) {
                intervalLength = distrFirstInterval(_gen);
                isFirstInterval = false; 
            } else {
                intervalLength = distrNormalInterval(_gen);
            }
            for (int i = 0; i < intervalLength; ++i) {
                Node* nextNode = get_random_child(node);
                if (!nextNode) {
                    reached_dead_end = true;
                    break;
                }
                node = nextNode;
                currentPart += node->letter();
            }

            wordParts.push_back(currentPart);

            if (reached_dead_end) {
                if (num_of_stars >= 1 && node->is_word) {
                    found_solution = true;
                    break;
                }
            } else {
                Node* nextNode = get_random_child(node);
                if (!nextNode) {
                    reached_dead_end = true;
                    if (num_of_stars >= 1 && node->is_word) {
                        found_solution = true; 
                    }
                    break;
                }
                node = nextNode;
                num_of_stars += 1; 
            }
        }
    }

    return wordParts;
}




















/*
This function deletes the data, thereby making sure 
 the next time a trie object gets reloaded, the trie will be empty.
(When the user clicks the "Delete Memory" button through the interface,
ResetTrie is called, and a new Trie instance is instantiated).
*/
void Operation::ResetTrie()
{
    std::ofstream stream;
    // stream.open(Operation::GetDataFile(), std::ofstream::trunc);
    // stream.close();
    stream.open(Operation::files_file(), std::ofstream::trunc);
    stream.close();

    _trie.set_counter(0);
    _suffixTrie.set_counter(0);

    _files_stored.clear();
}


/*
This function is called when the user inputs
 a word, or a prefix*, or a prefix*suffix, or a prefix*middle*suffix

 If the user used one star character (*), LookUpInTrie will call the
 memebr coroutine of class Trie SearchPreFixAndSuffixRoutine.

 If the user used two star characters (*)x2 , LookUpInTrie will call the 
 member coroutine of class Trie SearchPreMidSuffixesRoutine.

 If the user used zero star characters , LookUpInTrie will call the 
 member coroutine of class Trie SearchSubString.

 (The application does not let the user search for all words in trie, 
 because it is not efficent from the standpoint of Running Time efficiency)
*/
void Operation::LookUpInTrie(std::string word, std::vector<std::string>& wordsFound)
{
    int starCount = std::count(word.begin(), word.end(), '*');
    int spaceCount = std::count(word.begin(), word.end(), ' ');
    if (spaceCount > 0) {
        return;
    } else if (starCount == 1) {
        size_t pos = word.find('*');
        if (pos != std::string::npos) {
            if (word.length() == 1) {
                return;
            }
            if (pos > 0) {
                std::string beforeStar = word.substr(0, pos);
                std::string afterStar = word.substr(pos + 1);
                word_finder::ReplaceChars(beforeStar);
                word_finder::ReplaceChars(afterStar);
                Generator<std::pair<Node*, std::string>> gen = _trie.SearchPreFixAndSuffixRoutine(_trie.GetRootRaw(), beforeStar, afterStar, "");
                while (gen.next())
                {
                    std::pair<Node*, std::string> values = gen.getValue();
                    Node* nodePtr = values.first;
                    std::string word = values.second;
                    if (nodePtr != nullptr && !nodePtr->appearances().empty()) { 
                        wordsFound.push_back(word);
                        wordsFound.push_back("________________________________________________________");
                        for (auto& fileID : nodePtr->appearances())
                        {
                            wordsFound.push_back(word + " File: " + _files_stored[fileID]); 
                           // wordsFound.push_back(word + " File: " + _files_stored[pair.first] + "  Page: " + std::to_string(pair.second));               
                        }
                    }
                }
            } else {
                word.erase(word.begin());
                word_finder::ReplaceChars(word);
                std::string reversedWord = word;
                std::reverse(reversedWord.begin(), reversedWord.end());
                std::cout << "  " << reversedWord; 
                Generator<std::pair<Node*, std::string>> gen = _suffixTrie.SearchPreFixAndSuffixRoutine(_suffixTrie.GetRootRaw(), reversedWord, "", "");
                while (gen.next())
                {
                    std::pair<Node*, std::string> values = gen.getValue();
                    Node* nodePtr = values.first;
                    std::string word = values.second;
                    if (nodePtr != nullptr && !nodePtr->appearances().empty()) {
                        std::reverse(word.begin(), word.end());
                        wordsFound.push_back(word);
                        wordsFound.push_back("________________________________________________________");
                        for (auto& fileID : nodePtr->appearances())
                        {
                            wordsFound.push_back(word + " File: " + _files_stored[fileID]); 
                           // wordsFound.push_back(word + " File: " + _files_stored[pair.first] + "  Page: " + std::to_string(pair.second));               
                        }
                    }
                }               
            }
        }
    } else if (starCount == 2) {
        size_t pos1 = word.find('*');
        size_t pos2 = word.find('*', pos1 + 1);
        if (pos1 != std::string::npos && pos2 != std::string::npos) {
            if (pos1 > 0) {
                std::string prefix = word.substr(0, pos1);
                std::string midfix = word.substr(pos1 + 1, pos2 - pos1 -1);
                std::string suffix = word.substr(pos2 + 1);
                word_finder::ReplaceChars(prefix);
                word_finder::ReplaceChars(midfix);
                word_finder::ReplaceChars(suffix);
                Generator<std::pair<Node*, std::string>> gen = _trie.SearchPreMidSuffixesRoutine(_trie.GetRootRaw(), prefix, midfix, suffix);
                while (gen.next())
                {
                    std::pair<Node*, std::string> values = gen.getValue();
                    Node* nodePtr = values.first;
                    std::string word = values.second;
                    if (nodePtr != nullptr && !nodePtr->appearances().empty()) { 
                        wordsFound.push_back(word);                            
                        wordsFound.push_back("________________________________________________________");
                        for (auto& fileID : nodePtr->appearances())
                        {
                            wordsFound.push_back(word + " File: " + _files_stored[fileID]);
                           // wordsFound.push_back(word + " File: " + _files_stored[pair.first] + "  Page: " + std::to_string(pair.second));                 
                        }
                    }
                }
            }
        }
    } else if (starCount == 0) {
        if (word.length() > 0) {
            word_finder::ReplaceChars(word);
            std::pair<Node*, std::string> values = _trie.SearchSubString(_trie.GetRootRaw(), word, "");
            Node* nodePtr = values.first;
            std::string word = values.second;
            if (nodePtr != nullptr && !nodePtr->appearances().empty()) {
                wordsFound.push_back(word);
                wordsFound.push_back("________________________________________________________");
                for (auto& fileID : nodePtr->appearances())
                {
                     wordsFound.push_back(word + " File: " + _files_stored[fileID]);                 
                   // wordsFound.push_back(word + " File: " + _files_stored[pair.first] + "  Page: " + std::to_string(pair.second));                  
                }
            }
        }
    }
    
    return;
}


/**
 * Serialize trie to bin file
 */
void Operation::WriteTrie(std::filesystem::path filePath,  Trie& trie)
{
    std::ofstream stream(filePath, std::ios::binary | std::ios::trunc);
    std::deque<Node*> queue;

    queue.push_back(trie.GetRootRaw());

    while (!queue.empty())
    {
        Node* node = queue.front();
        queue.pop_front();
        node->Serialize(stream);

        std::deque<Node*> childrenQueue;
        for (char c : word_finder::kAlphabet)
        {
            if (node->child(c) != nullptr) {
                childrenQueue.push_front(node->child(c));
            }
        }
        while (!childrenQueue.empty())
        {
            queue.push_front(childrenQueue.front());
            childrenQueue.pop_front();
        }
    }
    stream.close();
    return;
}

/**
 * Read bin file and build the trie + bktree
 */
void Operation::ReadTrie(std::filesystem::path filePath, Trie& trie, bool insertToBKTree)
{
    std::cout << "entered function\n";
    std::ifstream stream1(Operation::files_file());
    std::string line1, file;
    if (stream1.is_open()) {
        while (std::getline(stream1, line1))
        {
            std::istringstream linestream1(line1);
            linestream1 >> file;

            int cnt = std::count(_files_stored.begin(), _files_stored.end(), file);
            if (cnt == 0) {
                 AddFile(file);
            }
        }
    }

    // if (trieIsPrefix)
    //     Node::set_counter(0);
    trie.set_counter(0);

    std::ifstream stream(filePath, std::ios::binary);
    std::deque<std::pair<Node*, std::string>> queue;

    queue.push_front(std::make_pair(trie.GetRootRaw(), ""));

    while (!queue.empty())
    {
        Node* node = queue.front().first;
        std::string path = queue.front().second;
        queue.pop_front();
        uint16_t size;
        // reading from the bin file the current node' amount of file ids.
        stream.read(reinterpret_cast<char*>(&size), sizeof(size));
        size = ntohs(size);
        // if size > 0 it means the current node represent a word.
        if (size > 0) {
            trie.IncrementCounter();
            if (insertToBKTree) {
                _bkTree.AddWordToTree(path);
            }
        }
        // if (size > 0 && trieIsPrefix) {
        //     Node::IncrementCounter();
        //     _bkTree.AddWordToTree(path);
        // }
        for (int i = 0; i < size; ++i)
        {
            uint16_t fileID;
            stream.read(reinterpret_cast<char*>(&fileID), sizeof(fileID));
            fileID = ntohs(fileID);
            node->AddAppearance(static_cast<unsigned short>(fileID));
        }
        char flag;
        std::deque<std::pair<Node*, std::string>> childrenQueue;
        for (char c : word_finder::kAlphabet)
        {
            std::string newPath = path + c;
            stream.read(&flag, sizeof(flag));
            if (flag == 1) {
                node->InsertChildLetter(c);
                childrenQueue.push_front(std::make_pair(node->child(c), newPath));
            }
        }
        while (!childrenQueue.empty())
        {
            queue.push_front(childrenQueue.front());
            childrenQueue.pop_front();
        }
    }
    stream.close();
    std::cout << "exited function\n";
}

void Operation::Serialize(std::filesystem::path directory)
{
    WriteTrie(directory / "trieCompressed.bin", _trie);
    WriteTrie(directory / "suffixtrieCompressed.bin", _suffixTrie);
}


 void Operation::LoadData(std::filesystem::path directory)
 {
    ReadTrie(directory / "trieCompressed.bin", _trie, true);
    ReadTrie(directory / "suffixtrieCompressed.bin", _suffixTrie, false);
 }