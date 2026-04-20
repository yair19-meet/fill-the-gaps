// trie.h contains the structure of the PreFix tree Data Structure (also known as trie).

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>
#include <deque>
#include <unordered_map>
#include <algorithm>
#include <fstream>

#include "generation.h"

/**
 * Class Node. each Node contains:
 * 1)  A static atomic Word Counter. TODO: move counter to class trie.
 *     The counter represents the amount of Nodes - representing a word - in the trie.
 * 2)  a letter.
 * 3)  a vector of file IDs - the files in which the word that the Node represents appear.
 * 4)  an unordered map of the Node's children.  
 *     Each key in the map is a letter and the value is the smart pointer of the child Node.
 * 5)  A mutex. The mutex is used when inserting a child. 
 *     
 *      
 * Note: Reading words and inserting them into the trie happens concurrently. 
 *       Two threads should not modify the same Node at the same time. Hence, the mutex.
 *       However, a node may be modified by one thread while another thread reads it.
 * 
 */
class Node
{
public:
    Node(char letter, bool is_word = false);
    char letter() const { return _letter; }
    bool is_word() const { return _is_word; }
    void set_is_word(bool is_word) { _is_word = is_word; }
    Node* child(char c) const;
    void set_child(std::shared_ptr<Node>&& node, char c) { _children.insert({c, std::move(node)}); }
    Node* InsertChildWord(char c); 
    void InsertChildLetter(char c);
    void Serialize(std::ofstream& stream);

private:
    char _letter;
    bool _is_word;
    std::unordered_map<char, std::shared_ptr<Node>> _children;
    std::mutex _mtx;
};

/**
 * Class Trie. 
 * 
 * The trie contains mapping of words to files that contain them.
 * For each word, there is a path from the root node that goes through a node for each letter of the word,
 * and the node with the last letter of the word contains a list of file IDs.
 * 
 * For example, if the word "bit" is added with file id 1, then:
 *  the root node will have a child node with the letter "b". 
 *  That node will have a child node with the letter "i".
 *  And that node will have a child with the letter "t" - this node will also contain file id 1.
 * 
 * Note: The trie supports adding words and searching for words. 
 *       Removing words is not supported.
 *       Adding words and searching for words can run concurrently.
 */
class Trie
{
public:
    Trie();
    Node* GetRootRaw() { return _root.get(); }
    Node* AddWordToTrie(const std::string& word);
    void SearchInTrie(std::string preFix, std::vector<std::string>& wordsFound);
    void DepthFirstSearch(Node* startNodePtr, std::vector<std::string>& wordsFound);
    std::pair<Node*, std::string> SearchSubString(Node* startNode, std::string fix, std::string word);
    Generator<std::pair<Node*, std::string>> SearchSuffixRoutine(Node* startNode, std::string suffix, std::string word);
    Generator<std::pair<Node*, std::string>> SearchPreFixAndSuffixRoutine(Node* startNode, 
                                std::string prefix, 
                                std::string suffix,
                                std::string word);
    Generator<std::pair<Node*, std::string>> SearchDoubleFixRoutine(Node* startNode, 
                        std::string middle, 
                        std::string suffix,
                        std::string word);
    Generator<std::pair<Node*, std::string>> SearchPreMidSuffixesRoutine(Node* startNode, 
                            std::string preFix, 
                            std::string middle,
                            std::string suffix);
    Node* AddSynonym(std::string& synonym);

private:
    std::unique_ptr<Node> const _root;
};