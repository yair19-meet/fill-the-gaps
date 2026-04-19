#pragma once

#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <filesystem>
#include <future>
#include <mutex>
#include <random>

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

#include "bktree.h"
#include "trie.h"


/**
 * Class Operation handles the high level operations of the program.
 * An instance of the class includes a prefix trie, a suffix trie and a bktree as the data structures.
 * the prefix and suffix trees are for searching for words, 
 * while the bktree is for the autocomplete functionality.
 * a class instance also includes a file path for storing the images names 
 * as well as a vector of the images names.
 * 
 * the class is meant to be singleton.
 */
class Operation
{
public:
    Operation();
    ~Operation();
    Trie& trie() { return _trie; }
    Trie& suffix_trie() { return _suffixTrie; }
    void LookUpInTrie(std::string preFix, std::vector<std::string>& wordsFound);
    void ResetTrie();
    void ReloadTrie(); 
    void Serialize(std::filesystem::path directory);
    void LoadData(std::filesystem::path directory);
    std::vector<std::string> GenerateBrokenWord();

private:
    void WriteTrie(std::filesystem::path filePath, Trie& trie);
    void ReadTrie(std::filesystem::path filePath, Trie& trie, bool trieIsPrefix); 
    // prefix tree for word search
    Trie _trie;
    // suffix tree for word search
    Trie _suffixTrie;

    std::mutex _mtx;

    std::random_device _gen;
};