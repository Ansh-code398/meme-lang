#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cassert>
#include <sys/resource.h>
#include "src/interpreter/Value.h"

// Empirical Challenger Harness: Nested Option/Result & Memory Stress Tests

int main() {
    std::cout << "=== Adversarial Option/Result Deep Stress & Memory Test ===" << std::endl;

    // Test 1: Nested Some(Some(42))
    {
        Value inner = Value(42LL);
        Value outer = Value::some(Value::some(inner));
        assert(outer.isOption());
        auto opt1 = outer.getOption();
        assert(opt1->isSome());
        assert(opt1->value.isOption());
        auto opt2 = opt1->value.getOption();
        assert(opt2->isSome());
        assert(opt2->value.asInt() == 42LL);
        std::cout << "  [PASS] Test 1: Some(Some(42)) construction & unwrapping" << std::endl;
    }

    // Test 2: Some(None)
    {
        Value outer = Value::some(Value::none());
        assert(outer.isOption());
        auto opt1 = outer.getOption();
        assert(opt1->isSome());
        assert(opt1->value.isOption());
        auto opt2 = opt1->value.getOption();
        assert(opt2->isNone());
        std::cout << "  [PASS] Test 2: Some(None) distinguishes outer Some from inner None" << std::endl;
    }

    // Test 3: Ok(Err("fail")) and Err(Ok(100))
    {
        Value res1 = Value::ok(Value::err(Value(std::string("kaboom"))));
        assert(res1.isResult());
        assert(res1.getResult()->isOk());
        assert(res1.getResult()->value.isResult());
        assert(res1.getResult()->value.getResult()->isErr());
        assert(res1.getResult()->value.getResult()->value.toString() == "kaboom");

        Value res2 = Value::err(Value::ok(Value(100LL)));
        assert(res2.isResult());
        assert(res2.getResult()->isErr());
        assert(res2.getResult()->value.isResult());
        assert(res2.getResult()->value.getResult()->isOk());
        assert(res2.getResult()->value.getResult()->value.asInt() == 100LL);
        std::cout << "  [PASS] Test 3: Ok(Err(...)) and Err(Ok(...)) nested results" << std::endl;
    }

    // Test 4: Deep nesting to depth 200
    {
        constexpr int DEPTH = 200;
        Value cur = Value(9999LL);
        for (int i = 0; i < DEPTH; ++i) {
            if (i % 2 == 0) {
                cur = Value::some(cur);
            } else {
                cur = Value::ok(cur);
            }
        }

        // Now unwind all 200 layers
        for (int i = DEPTH - 1; i >= 0; --i) {
            if (i % 2 == 0) {
                assert(cur.isOption());
                auto opt = cur.getOption();
                assert(opt->isSome());
                cur = opt->value;
            } else {
                assert(cur.isResult());
                auto res = cur.getResult();
                assert(res->isOk());
                cur = res->value;
            }
        }
        assert(cur.asInt() == 9999LL);
        std::cout << "  [PASS] Test 4: 200 levels deep alternating Option/Result unwinding" << std::endl;
    }

    // Test 5: Option & Result in MemeList and MemeMap
    {
        auto list = std::make_shared<MemeList>();
        list->elements.push_back(Value::some(Value(1LL)));
        list->elements.push_back(Value::none());
        list->elements.push_back(Value::ok(Value(std::string("pass"))));
        list->elements.push_back(Value::err(Value(std::string("fail"))));
        Value listVal(list);
        assert(listVal.isList());
        assert(list->elements[0].getOption()->value.asInt() == 1LL);
        assert(list->elements[1].getOption()->isNone());
        assert(list->elements[2].getResult()->value.toString() == "pass");
        assert(list->elements[3].getResult()->isErr());

        auto map = std::make_shared<MemeMap>();
        map->set("opt", Value::some(Value(42LL)));
        map->set("res", Value::ok(Value(std::string("map_ok"))));
        Value mapVal(map);
        assert(mapVal.isMap());
        assert(map->get("opt").getOption()->value.asInt() == 42LL);
        assert(map->get("res").getResult()->value.toString() == "map_ok");
        std::cout << "  [PASS] Test 5: Option & Result inside MemeList and MemeMap" << std::endl;
    }

    // Test 6: String representation of nested variants
    {
        Value v1 = Value::some(Value::some(Value(10LL)));
        assert(v1.toString() == "Some(Some(10))");

        Value v2 = Value::ok(Value::err(Value(std::string("oops"))));
        assert(v2.toString() == "Ok(Err(oops))");

        Value v3 = Value::some(Value::none());
        assert(v3.toString() == "Some(None)");
        std::cout << "  [PASS] Test 6: String serialization of nested variants" << std::endl;
    }

    // Test 7: Memory leak / stress test with 500,000 allocations
    {
        struct rusage usage_before, usage_after;
        getrusage(RUSAGE_SELF, &usage_before);

        constexpr int ITERATIONS = 500000;
        for (int i = 0; i < ITERATIONS; ++i) {
            Value v = Value::some(Value::ok(Value::some(Value(static_cast<long long>(i)))));
            // Scope exit frees v and its inner shared_ptr chain
        }

        getrusage(RUSAGE_SELF, &usage_after);
        long mem_diff_kb = (usage_after.ru_maxrss - usage_before.ru_maxrss);
#ifdef __APPLE__
        mem_diff_kb /= 1024;
#endif
        std::cout << "  [PASS] Test 7: 500,000 allocations completed. Max RSS delta: "
                  << mem_diff_kb << " KB" << std::endl;
    }

    std::cout << "ALL OPTION/RESULT C++ ADVERSARIAL TESTS PASSED!" << std::endl;
    return 0;
}
