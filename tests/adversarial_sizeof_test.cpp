#include <iostream>
#include <iomanip>
#include <string>
#include <variant>
#include <memory>
#include <cassert>
#include "src/interpreter/Value.h"

int main() {
    std::cout << "=== Adversarial sizeof(Value) Inspection ===" << std::endl;
    std::cout << "sizeof(Value):               " << sizeof(Value) << " bytes" << std::endl;
    std::cout << "alignof(Value):              " << alignof(Value) << " bytes" << std::endl;
    std::cout << "sizeof(ValueVariant):        " << sizeof(ValueVariant) << " bytes" << std::endl;
    std::cout << "alignof(ValueVariant):       " << alignof(ValueVariant) << " bytes" << std::endl;
    std::cout << "sizeof(MemeOption):          " << sizeof(MemeOption) << " bytes" << std::endl;
    std::cout << "sizeof(MemeResult):          " << sizeof(MemeResult) << " bytes" << std::endl;
    std::cout << "sizeof(std::shared_ptr<MemeOption>): " << sizeof(std::shared_ptr<MemeOption>) << " bytes" << std::endl;
    std::cout << "sizeof(std::shared_ptr<MemeResult>): " << sizeof(std::shared_ptr<MemeResult>) << " bytes" << std::endl;
    std::cout << "sizeof(std::string):         " << sizeof(std::string) << " bytes" << std::endl;
    std::cout << "sizeof(long long):           " << sizeof(long long) << " bytes" << std::endl;
    std::cout << "sizeof(double):              " << sizeof(double) << " bytes" << std::endl;

#if defined(__EMSCRIPTEN__) || defined(__wasm32__) || (INTPTR_MAX == INT32_MAX)
    constexpr size_t EXPECTED_SIZE = 24;
#else
    constexpr size_t EXPECTED_SIZE = 32;
#endif

    if (sizeof(Value) != EXPECTED_SIZE) {
        std::cerr << "INVARIANT VIOLATION: sizeof(Value) is " << sizeof(Value)
                  << ", expected " << EXPECTED_SIZE << std::endl;
        return 1;
    }

    // Verify alignment
    if (alignof(Value) != 8) {
        std::cerr << "ALIGNMENT WARNING: alignof(Value) is " << alignof(Value)
                  << ", expected 8" << std::endl;
        return 2;
    }

    std::cout << "SUCCESS: sizeof(Value) == " << EXPECTED_SIZE << " verified." << std::endl;
    return 0;
}
