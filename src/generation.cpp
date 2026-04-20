#include <coroutine>
#include <memory>
#include <iostream>
#include <deque>

#include "trie.h"

inline constexpr std::string_view kAlphabet = "abcdefghijklmnopqrstuvwxyz";

Generator<std::string> autoComplete(Node* startNode, std::string word)
{
    std::deque<std::pair<Node*, std::string>> queue;
    Node* nodePtr = startNode;
    std::string letters = word;
    if (nodePtr == nullptr) {
        // while(true)
        // {
        //     co_yield "";
        // }
        co_return;
    }
    while (true)
    {
        for (char c : kAlphabet)
        {
            Node* childPtr = nodePtr->child(c);
            std::string wordPlus = letters + c;
            if (childPtr != nullptr) {
                queue.push_back(std::make_pair(childPtr, wordPlus));
                if (childPtr->is_word()) {
                    co_yield wordPlus;
                } 
            } 
        }
        if (queue.empty()) {
            co_yield "";
            nodePtr = startNode;
            letters = word;
        }  else {
            nodePtr = queue.front().first;
            letters = queue.front().second;
            queue.pop_front();
        }
    }
}

