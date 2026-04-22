#include <emscripten/bind.h>
#include <vector>
#include <string>
#include "operation.h"

using namespace emscripten;

// Helper struct for validation results to make JS consumption easier
struct ValidationResult {
    bool valid;
    std::string correctWord;
};

// Wrapper function to handle the std::pair return from checkWordValidity
ValidationResult checkWordValidityWrapper(Operation& op, std::string guess, std::vector<std::string> brokenWord) {
    auto result = op.checkWordValidity(guess, brokenWord);
    std::string correct = "";
    if (!result.first && !result.second.empty()) {
        // Pick a random correct word if guess was wrong
        std::uniform_int_distribution<> distr(0, result.second.size() - 1);
        correct = result.second[distr(op.GetRand())];
    }
    return { result.first, correct };
}

EMSCRIPTEN_BINDINGS(game_engine) {
    register_vector<std::string>("StringVector");

    value_object<ValidationResult>("ValidationResult")
        .field("valid", &ValidationResult::valid)
        .field("correctWord", &ValidationResult::correctWord);

    class_<Operation>("Operation")
        .constructor<>()
        .function("LoadDictionary", &Operation::LoadDictionary)
        .function("GenerateBrokenWord", &Operation::GenerateBrokenWord)
        .function("checkWordValidity", &checkWordValidityWrapper);
}
