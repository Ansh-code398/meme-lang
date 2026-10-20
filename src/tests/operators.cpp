#include <iostream>
#include <limits>
#include "../interpreter/Value.h"
#include "../interpreter/Operators.h"

// Helper to format Value for test output
std::string formatValue(const Value &v)
{
    return std::visit(
        overload{
            [](std::monostate) -> std::string { return "kuch_nhi"; },
            [](long long x) -> std::string { return std::to_string(x); },
            [](double x) -> std::string { return std::to_string(x); },
            [](const std::string &x) -> std::string { return "\"" + x + "\""; },
            [](bool x) -> std::string { return x ? "han" : "nhi"; },
            [](const std::shared_ptr<Callable> &x) -> std::string { return x ? x->toString() : "<function>"; }
        },
        v.data);
}

void testAdd(const std::string &label, const Value &a, const Value &b)
{
    std::cout << "=== Test: " << label << " ===\n";
    try
    {
        Value res = add(a, b);
        std::cout << "Result: " << formatValue(res)
                  << " (type: " << res.typeName() << ")\n\n";
    }
    catch (const std::exception &e)
    {
        std::cout << "Exception: " << e.what() << "\n\n";
    }
}

int main()
{
    // 1. int + int -> int
    testAdd("1 + 2", Value(1LL), Value(2LL));

    // 2. int + float -> float
    testAdd("1 + 2.5", Value(1LL), Value(2.5));

    // 3. string + string -> string concatenation
    testAdd("\"a\" + \"b\"", Value("a"), Value("b"));

    // 4. int + string -> Type Error
    testAdd("1 + \"a\"", Value(1LL), Value("a"));

    // 5. LLONG_MAX + 1 -> Integer Overflow Error
    testAdd("LLONG_MAX + 1 (Overflow)",
            Value(std::numeric_limits<long long>::max()),
            Value(1LL));

    return 0;
}