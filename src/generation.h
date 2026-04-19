#ifndef WORD_FINDER_GENERATION_H
#define WORD_FINDER_GENERATION_H
#include <coroutine>
#include <memory>
#include <iostream>

class Node;

template<typename T>
struct Generator
{
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type co;
    Generator(handle_type h) : co(h) {}
    ~Generator()
    {
        if (co)
            co.destroy();
    }
    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;
    Generator(Generator&& other) noexcept
    {
        co = other.co;
        other.co = nullptr;
    }
    Generator& operator=(Generator&& other) noexcept
    {
        co = other.co;
        other.co = nullptr;
        return *this;
    }
    T getValue()
    {
        return co.promise().current_value;
    }
    bool next()
    {
        co.resume();
        return not co.done();
    }
    struct promise_type
    {
        promise_type() = default;
        ~promise_type() = default;
        T current_value;
        auto initial_suspend()
        {
            return std::suspend_always{};
        }
        auto final_suspend() noexcept
        {
            return std::suspend_always{};
        }
        auto get_return_object()
        {
            return Generator{handle_type::from_promise(*this)};
        }
        auto return_void()
        {
            //return std::suspend_never{};
        }
        auto yield_value(const T value)
        {
            current_value = value;
            return std::suspend_always{};
        }
        void unhandled_exception()
        {
            
        }
    };
    
};

Generator<std::string> autoComplete(Node* startNode, std::string word);


#endif /* WORD_FINDER_GENERATION_H */