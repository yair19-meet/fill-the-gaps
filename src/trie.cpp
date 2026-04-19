#include "trie.h"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>

#include <arpa/inet.h>

#include "generation.h"

inline constexpr std::string_view kAlphabet = "abcdefghijklmnopqrstuvwxyz";


// Initializing the Counter
std::atomic<uint32_t> Node::_WordCounter = 0;

// Constructor for creating a Node which is a part of a path to a word
Node::Node(char letter, bool is_word) : _letter(letter), _is_word(is_word) {}

Node* Node::child(char c) const { 
    auto it = _children.find(c); 
    if (it == _children.end()) {
        return nullptr;
    }
    return _children.at(c).get();
}

unsigned short Node::GetLastAppearance() const
{
    if (_appearances.empty()) {
        throw std::runtime_error("No appearances available");
    }
    return _appearances.back();
}


void Node::Serialize(std::ofstream& stream)
{
    uint16_t size = htons(static_cast<uint16_t>(_appearances.size()));
    stream.write(reinterpret_cast<const char*>(&size), sizeof(size));

    for (auto& fileID : _appearances)
    {
       uint16_t fID = htons(fileID);
        stream.write(reinterpret_cast<const char*>(&fID), sizeof(fID));       
    }
    
    for (char c : word_finder::kAlphabet)
    {
        char flag = (child(c) == nullptr) ? 0 : 1;
        stream.write(&flag, sizeof(flag));
    }
}



/*
The following function is responsible for updating the trie
 when encountering a new word in a file and reaching the end of path in trie.
In case the full path to the word does not exist already, 
the smart pointer of object Node which is passed to the function as "node" 
(as an rvalue reference)
 is inserted into the trie using move semantics.
In case the path to the word already exist in the trie, 
the function simply calls the auxilliary function "AddAppearance".
Moreover, in a situation where the node exist already 
but it is a part of a longer path and does not represent a word,
 _word is set to the new word.

 A lock_guard is used in order to ensure that there are no multiple threads
 modifying the same Node object at once.
*/
Node* Node::InsertChildWord(char c, bool is_word) 
{ 
    std::lock_guard<std::mutex> lck(_mtx);
    if (this->child(c) == nullptr) {
        std::shared_ptr<Node> newWordNode = std::make_shared<Node>(c, true);
        this->set_child(std::move(newWordNode), c); 
        return this->child(c);
    } else {
        if (this->child(c)->appearances().empty()) {
            this->child(c)->set_is_word(true);
            return this->child(c);
        }  
    }
    return nullptr;
}

/*
This function is responsible for inserting a Node 
to represent a letter in a path to a word.
A lock_guard is used in order to ensure that there are no multiple threads
 modifying the same Node object at once.
*/
void Node::InsertChildLetter(char c)
{
    std::lock_guard<std::mutex> lck(_mtx);
    if (this->child(c) == nullptr) {
        std::unique_ptr<Node> newNode = std::make_unique<Node>(c, false);
        this->set_child(std::move(newNode), c);
    } 
    return;
}


//Constructor for creating a trie
Trie::Trie() : _root(std::make_unique<Node>('*')) {}


// Function for creating a path to a word in trie
Node* Trie::AddWordToTrie(const std::string& word, unsigned short fileID)
{
    Node* nodePtr = _root.get();
    for (int i = 0; i < word.length() - 1; ++i)
    {
        char c = word[i];
        nodePtr->InsertChildLetter(c);
        nodePtr = nodePtr->child(c);
    }
    char lastLetter = word[word.length() - 1];

    //Uncomment lines 114 - 117 in order for the threads to pause
    // before inserting a new Node.
    /*
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, 100);
    std::this_thread::sleep_for(std::chrono::milliseconds(distrib(gen)));
    */

    Node* node = nodePtr->InsertChildWord(lastLetter);
    return node;
}

// Node* Trie::AddSynonym(std::string& synonym)
// {
//     Node* nodePtr = _root.get();
//     for (int i = 0; i < synonym.length(); ++i)
//     {
//         char c = synonym[i];
//         nodePtr->InsertChildLetter(c);
//         nodePtr = nodePtr->child(c);
//     }
//     nodePtr->set_word(synonym);
//     return nodePtr;
// }


/* 
 The following function is responsible for travesing the trie in order to find a substring.
 It returns the pointer to the Node that is the end of the path to the substring.
 In case the path does not exist, the function returns nullptr.
*/
std::pair<Node*, std::string> Trie::SearchSubString(Node* startNode, std::string fix, std::string word)
{
    Node* nodePtr = startNode;
    std::string letters = word;
    for (int i = 0; i < fix.length(); ++i)
    {
        char c = fix[i];
        if (nodePtr->child(c) == nullptr) {
            return std::make_pair(nullptr, "");
        } 
        nodePtr = nodePtr->child(c);
        letters += c;
    }
    return std::make_pair(nodePtr, letters);
}


/*
The following function yields all words in trie that end with the suffix, 
starting the search from startNode.

The coroutine has a queue for tracking nodes to be expanded.
A the beginning the only node in queue is startNode.

It calls the function "SearchSubString" with passing:
1) the node where we are currently in the trie as "currentPtr"
2) the suffix

if the path of the suffix exist in the trie, starting from "currentPtr",
and it is a word, the we yield the pointer to the Node
representing the word.

Then we push all the non-null children of currentPtr to the queue,
and the loop goes on.

*Be aware that this function is not meant to be called with startNode=root. 
We do not want to traverse the entire trie in search for a suffix. 
That is NOT Runtime efficent.


*/

Generator<std::pair<Node*, std::string>> Trie::SearchSuffixRoutine(Node* startNode, std::string suffix, std::string word)
{
    std::deque<std::pair<Node*, std::string>> queue;
    queue.push_back(std::make_pair(startNode, word));
    while (!queue.empty())
    {
        Node* currentPtr = queue.front().first;
        std::string letters = queue.front().second;
        queue.pop_front();
        std::pair<Node*, std::string> values = Trie::SearchSubString(currentPtr, suffix, letters);
        Node* nodePtr = values.first;
        std::string fullWord = values.second;

        if (nodePtr != nullptr && !nodePtr->appearances().empty()) {
            co_yield std::make_pair(nodePtr, fullWord);
        }
        for (char c : word_finder::kAlphabet)
        {
            Node* childPtr = currentPtr->child(c);
            std::string wordPlus = letters + c;
            if (childPtr != nullptr) {
                queue.push_back(std::make_pair(childPtr, wordPlus));
            } 
        }
    }
    co_return;
}

/*
The following coroutine searches for all words in trie that end with the suffix
and contains a substring (passed as the variable middle), 
starting the search from startNode.

The coroutine has a queue for tracking nodes to be expanded.
A the beginning the only node in queue is startNode.

It calls the coroutine "SearchPreFixAndSuffix" with passing:
1) the node where we are currently in the trie
2) middle as the prefix
3) suffix as the suffix

It yields the Node pointers that it gets from SearchPreFixAndSuffix.
Then it pushes the non-null children of the current Node to the queue,
and the loop goes on.

*/
Generator<std::pair<Node*, std::string>> Trie::SearchDoubleFixRoutine(Node* startNode, 
                           std::string middle, 
                           std::string suffix,
                           std::string word)
{
    std::deque<std::pair<Node*, std::string>> queue;
    queue.push_back(std::make_pair(startNode, word));
    while (!queue.empty())
    {
        Node* currentPtr = queue.front().first;
        std::string letters = queue.front().second;
        queue.pop_front();
        Generator<std::pair<Node*, std::string>> gen = Trie::SearchPreFixAndSuffixRoutine(currentPtr, middle, suffix, letters);
        while (gen.next())
        {
            co_yield gen.getValue();
        }
        for (char c : word_finder::kAlphabet)
        {
            Node* childPtr = currentPtr->child(c);
            std::string wordPlus = letters + c;
            if (childPtr != nullptr) {
               queue.push_back(std::make_pair(childPtr, wordPlus));
            }
        }
    }
    co_return;
}

/*
The following coroutine searches for words that start with prefix and end with suffix,
starting the search from startNode;
*/
Generator<std::pair<Node*, std::string>> Trie::SearchPreFixAndSuffixRoutine(Node* startNode, 
                                 std::string prefix, 
                                 std::string suffix,
                                 std::string word)
{
    std::pair<Node*, std::string> values = Trie::SearchSubString(startNode, prefix, word);
    Node* endOfPreFix = values.first;
    std::string fullWord = values.second;
    if (endOfPreFix != nullptr) {
        auto gen = Trie::SearchSuffixRoutine(endOfPreFix, suffix, fullWord);
        while (gen.next())
        {
            co_yield gen.getValue();
        }
    }
    co_return;
}

/*
The following coroutine searches for words that start with prefix, end with suffix, 
and have a substring in the middle which is passed as the variable middle.
It starts the search from startNode.
*/
Generator<std::pair<Node*, std::string>> Trie::SearchPreMidSuffixesRoutine(Node* startNode, 
                                std::string preFix, 
                                std::string middle,
                                std::string suffix)
{
    std::pair<Node*, std::string> values = Trie::SearchSubString(startNode, preFix, "");
    Node* endOfPreFix = values.first;
    std::string letters = values.second;

    if (endOfPreFix != nullptr) {
        Generator<std::pair<Node*, std::string>> gen = Trie::SearchDoubleFixRoutine(endOfPreFix, middle, suffix, letters);
        while (gen.next())
        {
            co_yield gen.getValue();
        }
    }
    co_return;
}