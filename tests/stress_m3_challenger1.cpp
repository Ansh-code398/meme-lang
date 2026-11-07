#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <memory>
#include <sstream>
#include "src/lexer/Lexer.h"
#include "src/parser/Parser.h"
#include "src/types/TypeChecker.h"
#include "src/types/Type.h"
#include "src/interpreter/Value.h"

// ============================================================================
// CHALLENGER 1 EMPIRICAL STRESS TEST HARNESS — MILESTONE 3
// Focus: Synthetic Overload Resolution & Pointer Subtyping Fuzzing
// ============================================================================

int testsPassed = 0;
int testsFailed = 0;

void recordPass(const std::string &name) {
    testsPassed++;
    std::cout << "  [PASS] " << name << std::endl;
}

void recordFail(const std::string &name, const std::string &reason) {
    testsFailed++;
    std::cerr << "  [FAIL] " << name << " -> " << reason << std::endl;
}

#define TEST_ASSERT(cond, name, msg) do { \
    if (cond) { recordPass(name); } \
    else { recordFail(name, msg); } \
} while(0)

// Helper: Run Lexer + Parser + TypeChecker
static bool checkTypeOnly(const std::string &code)
{
    try
    {
        Lexer lexer(code);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto program = parser.parseProgram();

        TypeChecker typeChecker;
        typeChecker.check(program);
        return true;
    }
    catch (const std::exception &e)
    {
        return false;
    }
}

// Helper: Expect exact substring in TypeChecker exception
static bool expectTypeError(const std::string &code, const std::string &expectedSubstring, std::string &actualError)
{
    try
    {
        Lexer lexer(code);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto program = parser.parseProgram();

        TypeChecker typeChecker;
        typeChecker.check(program);
        actualError = "No exception was thrown!";
        return false;
    }
    catch (const std::exception &e)
    {
        actualError = e.what();
        return actualError.find(expectedSubstring) != std::string::npos;
    }
}

// ============================================================================
// SUITE 1: Value Struct Invariant & Pointer Value Representation
// ============================================================================
void testValueAndMemoryInvariants()
{
    std::cout << "\n=== SUITE 1: Value Struct & Pointer Memory Invariants ===" << std::endl;

    // 1. static_assert compile-time size invariant
    static_assert(sizeof(Value) == 32, "Value size must strictly be 32 bytes on 64-bit systems!");
    TEST_ASSERT(sizeof(Value) == 32, "sizeof(Value) == 32 runtime verification", "Expected 32 bytes");

    // 2. Null Pointer creation and invariants
    auto constNull = Value::nullPointer(false);
    TEST_ASSERT(constNull.isPointer(), "constNull.isPointer()", "Expected isPointer() true");
    TEST_ASSERT(constNull.typeName() == "patta", "constNull.typeName() == 'patta'", "Expected 'patta'");
    TEST_ASSERT(constNull.toString() == "patta[0x0]", "constNull.toString() == 'patta[0x0]'", "Expected patta[0x0]");
    TEST_ASSERT(!constNull.isTruthy(), "!constNull.isTruthy()", "Null pointer must be falsy");
    TEST_ASSERT(constNull.asPointer() != nullptr, "constNull.asPointer() != nullptr", "Expected non-null Pointer struct");
    TEST_ASSERT(!constNull.asPointer()->isMut, "!constNull.asPointer()->isMut", "Expected isMut == false");
    TEST_ASSERT(constNull.asPointer()->isNull(), "constNull.asPointer()->isNull()", "Expected isNull() == true");
    TEST_ASSERT(constNull.asPointer()->getAddress() == 0, "constNull.asPointer()->getAddress() == 0", "Expected address 0");

    auto mutNull = Value::nullPointer(true);
    TEST_ASSERT(mutNull.isPointer(), "mutNull.isPointer()", "Expected isPointer() true");
    TEST_ASSERT(mutNull.asPointer()->isMut, "mutNull.asPointer()->isMut", "Expected isMut == true");

    // 3. Non-null Pointer creation
    long long dummyTarget = 12345;
    Pointer rawPtr(reinterpret_cast<Value*>(&dummyTarget), true, "target_var");
    auto nonNullVal = Value::pointer(rawPtr);
    TEST_ASSERT(nonNullVal.isPointer(), "nonNullVal.isPointer()", "Expected isPointer() true");
    TEST_ASSERT(nonNullVal.typeName() == "patta", "nonNullVal.typeName() == 'patta'", "Expected 'patta'");
    TEST_ASSERT(nonNullVal.isTruthy(), "nonNullVal.isTruthy()", "Non-null pointer must be truthy");
    TEST_ASSERT(nonNullVal.toString().find("patta[0x") == 0, "nonNullVal.toString() starts with 'patta[0x'", "Format must be patta[0x...]");
    TEST_ASSERT(nonNullVal.asPointer()->getAddress() == reinterpret_cast<uintptr_t>(&dummyTarget), "nonNullVal address matches", "Address mismatch");

    // 4. Pointer Equality
    auto p1 = Value::pointer(rawPtr);
    auto p2 = Value::pointer(rawPtr);
    TEST_ASSERT(p1.pointerEquals(p2), "p1.pointerEquals(p2)", "Pointers with same target must be equal");

    long long otherTarget = 67890;
    Pointer otherRawPtr(reinterpret_cast<Value*>(&otherTarget), true, "other_var");
    auto pOther = Value::pointer(otherRawPtr);
    TEST_ASSERT(!p1.pointerEquals(pOther), "!p1.pointerEquals(pOther)", "Pointers to different targets must not be equal");
}

// ============================================================================
// SUITE 2: Pointer Type Subtyping & Mutability Invariants
// ============================================================================
void testPointerTypeSubtypingAndAssignability()
{
    std::cout << "\n=== SUITE 2: Pointer Type Subtyping & Mutability Invariants ===" << std::endl;

    auto pConstAnk = Type::pointer(Type::Ank(), false);
    auto pMutAnk = Type::pointer(Type::Ank(), true);
    auto pConstBol = Type::pointer(Type::Bol(), false);
    auto pMutBol = Type::pointer(Type::Bol(), true);
    auto pConstPoint = Type::pointer(Type::Point(), false);
    auto pMutPoint = Type::pointer(Type::Point(), true);

    // 1. Type queries
    TEST_ASSERT(pConstAnk->isPointer(), "pConstAnk->isPointer()", "Expected isPointer() true");
    TEST_ASSERT(!pConstAnk->isMutablePointer(), "!pConstAnk->isMutablePointer()", "Expected false for const");
    TEST_ASSERT(pMutAnk->isPointer(), "pMutAnk->isPointer()", "Expected isPointer() true");
    TEST_ASSERT(pMutAnk->isMutablePointer(), "pMutAnk->isMutablePointer()", "Expected true for mut");

    // 2. String representation
    TEST_ASSERT(pConstAnk->toString() == "patta[ank]", "pConstAnk->toString() == 'patta[ank]'", "Format mismatch");
    TEST_ASSERT(pMutAnk->toString() == "patta[badal ank]", "pMutAnk->toString() == 'patta[badal ank]'", "Format mismatch");

    // 3. Structural Equality
    TEST_ASSERT(pConstAnk->equals(*Type::pointer(Type::Ank(), false)), "pConstAnk equals identical type", "Expected equals");
    TEST_ASSERT(!pConstAnk->equals(*pMutAnk), "!pConstAnk equals pMutAnk", "Const and mut pointers are not equal");
    TEST_ASSERT(!pConstAnk->equals(*pConstBol), "!pConstAnk equals pConstBol", "Different pointee types not equal");

    // 4. Mutability Subtyping:
    // patta[badal ank] is assignable to patta[ank] (permission weakening: safe!)
    TEST_ASSERT(pMutAnk->isAssignableTo(*pConstAnk), "patta[badal ank] assignable to patta[ank]", "Permission weakening must be allowed");

    // patta[ank] is NOT assignable to patta[badal ank] (permission escalation: unsafe!)
    TEST_ASSERT(!pConstAnk->isAssignableTo(*pMutAnk), "patta[ank] NOT assignable to patta[badal ank]", "Permission escalation must be forbidden");

    // 5. Invariant pointee for mutable pointers:
    // Even though byte widens to ank, patta[badal byte] is NOT assignable to patta[badal ank]!
    auto pMutByte = Type::pointer(Type::Byte(), true);
    TEST_ASSERT(!pMutByte->isAssignableTo(*pMutAnk), "patta[badal byte] NOT assignable to patta[badal ank]", "Mutable pointer requires invariant pointee");

    // 6. Covariant pointee for const pointers:
    // For const pointers, covariant pointee widening IS safe (read-only):
    auto pConstByte = Type::pointer(Type::Byte(), false);
    TEST_ASSERT(pConstByte->isAssignableTo(*pConstAnk), "patta[byte] assignable to patta[ank]", "Const pointer allows covariant pointee");

    // 7. Incompatible pointee types
    TEST_ASSERT(!pMutAnk->isAssignableTo(*pMutBol), "!pMutAnk->isAssignableTo(*pMutBol)", "Incompatible pointee must fail");
    TEST_ASSERT(!pConstAnk->isAssignableTo(*pConstBol), "!pConstAnk->isAssignableTo(*pConstBol)", "Incompatible pointee must fail");

    // 8. Nullable Reference: Nalla assignable to pointer
    TEST_ASSERT(Type::Nalla()->isAssignableTo(*pConstAnk), "nalla assignable to patta[ank]", "Expected true");
    TEST_ASSERT(Type::Nalla()->isAssignableTo(*pMutAnk), "nalla assignable to patta[badal ank]", "Expected true");

    // 9. Pointer not assignable to non-pointer and vice-versa
    TEST_ASSERT(!pConstAnk->isAssignableTo(*Type::Ank()), "!pConstAnk assignable to ank", "Pointer cannot be assigned to value");
    TEST_ASSERT(!Type::Ank()->isAssignableTo(*pConstAnk), "!ank assignable to pConstAnk", "Value cannot be assigned to pointer");

    // 10. TypeChecker static enforcement of pointer subtyping
    std::string passWeakening = R"raw(
        pakad x: ank = 42;
        pakad p_mut: patta[badal ank] = &x;
        pakad p_const: patta[ank] = p_mut;
    )raw";
    TEST_ASSERT(checkTypeOnly(passWeakening), "TypeChecker: mutable pointer assignable to const pointer", "Failed static check");

    std::string failEscalation = R"raw(
        pakad atal x: ank = 42;
        pakad p_const: patta[ank] = &x;
        pakad p_mut: patta[badal ank] = p_const;
    )raw";
    std::string err;
    bool rejected = expectTypeError(failEscalation, "Aukaat ke bahar!", err);
    TEST_ASSERT(rejected, "TypeChecker: const pointer NOT assignable to mutable pointer", "Failed to reject escalation: " + err);
}

// ============================================================================
// SUITE 3: Arity-Based Overloading (0, 1, 2, 3 args)
// ============================================================================
void testArityBasedOverloading()
{
    std::cout << "\n=== SUITE 3: Arity-Based Overloading (0, 1, 2, 3 args) ===" << std::endl;

    // 1. Same function name overloaded for 0, 1, 2, 3 arguments
    std::string aritySuite = R"raw(
        bahuroop kar compute(): ank { ye_le 0; }
        bahuroop kar compute(a: ank): ank { ye_le a; }
        bahuroop kar compute(a: ank, b: ank): ank { ye_le a + b; }
        bahuroop kar compute(a: ank, b: ank, c: ank): ank { ye_le a + b + c; }

        compute();
        compute(10);
        compute(10, 20);
        compute(10, 20, 30);
    )raw";
    TEST_ASSERT(checkTypeOnly(aritySuite), "TypeChecker: compute() overloads with 0, 1, 2, 3 args resolve", "Failed arity resolution");

    // 2. Distinct return types per arity verified via typed variable assignment
    std::string arityWithReturnTypes = R"raw(
        bahuroop kar poly_ret(): bol { ye_le "zero"; }
        bahuroop kar poly_ret(a: ank): ank { ye_le a * 2; }
        bahuroop kar poly_ret(a: ank, b: ank): sach { ye_le han; }
        bahuroop kar poly_ret(a: ank, b: ank, c: ank): point { ye_le 1.23; }

        pakad r0: bol = poly_ret();
        pakad r1: ank = poly_ret(21);
        pakad r2: sach = poly_ret(1, 2);
        pakad r3: point = poly_ret(1, 2, 3);
    )raw";
    TEST_ASSERT(checkTypeOnly(arityWithReturnTypes), "TypeChecker: distinct return types matched per arity", "Failed return type check");

    // 3. Calling with unhandled arity (e.g. 4 args) rejected with argument count mismatch
    std::string invalidArity = R"raw(
        bahuroop kar process(): ank { ye_le 0; }
        bahuroop kar process(a: ank): ank { ye_le a; }
        bahuroop kar process(a: ank, b: ank): ank { ye_le a + b; }
        process(1, 2, 3, 4);
    )raw";
    std::string err;
    bool rejectedCount = expectTypeError(invalidArity, "Aukaat ke bahar! Jugaad 'process' ke liye koi matching overload nahi mila (argument count mismatch)!", err);
    TEST_ASSERT(rejectedCount, "TypeChecker: 4-arg call rejected with arity mismatch diagnostic", "Error was: " + err);

    // 4. English keyword alias 'overload'
    std::string englishOverload = R"raw(
        overload kar multi_test(): ank { ye_le 100; }
        overload kar multi_test(x: ank): ank { ye_le x; }
        overload kar multi_test(x: ank, y: ank): ank { ye_le x + y; }
        pakad v0: ank = multi_test();
        pakad v1: ank = multi_test(50);
        pakad v2: ank = multi_test(30, 70);
    )raw";
    TEST_ASSERT(checkTypeOnly(englishOverload), "TypeChecker: English 'overload' keyword functions identically", "Failed English overload test");

    // 5. Desi meme keyword alias 'aur_ek'
    std::string desiAliasOverload = R"raw(
        bahuroop kar step(): ank { ye_le 1; }
        aur_ek kar step(a: ank): ank { ye_le a; }
        ek_aur kar step(a: ank, b: ank): ank { ye_le a + b; }
        step();
        step(10);
        step(10, 20);
    )raw";
    TEST_ASSERT(checkTypeOnly(desiAliasOverload), "TypeChecker: Desi meme aliases 'aur_ek' and 'ek_aur' resolve", "Failed Desi alias test");
}

// ============================================================================
// SUITE 4: Type-Based Overloading (ank, bol, sach, patta[ank])
// ============================================================================
void testTypeBasedOverloading()
{
    std::cout << "\n=== SUITE 4: Type-Based Overloading (ank, bol, sach, patta[ank]) ===" << std::endl;

    // 1. Overload set with distinct types f(ank), f(bol), f(sach), f(patta[ank])
    std::string typeOverloadSuite = R"raw(
        bahuroop kar inspect_type(x: ank): bol { ye_le "ank"; }
        bahuroop kar inspect_type(x: bol): bol { ye_le "bol"; }
        bahuroop kar inspect_type(x: sach): bol { ye_le "sach"; }
        bahuroop kar inspect_type(x: patta[ank]): bol { ye_le "patta_ank"; }

        pakad t1: bol = inspect_type(42);
        pakad t2: bol = inspect_type("Namaste");
        pakad t3: bol = inspect_type(han);

        pakad my_val: ank = 999;
        pakad p_ank: patta[ank] = &my_val;
        pakad t4: bol = inspect_type(p_ank);
    )raw";
    TEST_ASSERT(checkTypeOnly(typeOverloadSuite), "TypeChecker: f(ank), f(bol), f(sach), f(patta[ank]) all resolve", "Failed type dispatch");

    // 2. Mutable pointer passed to f(patta[ank]) resolves via permission weakening
    std::string mutPointerToConstParam = R"raw(
        bahuroop kar inspect_type(x: ank): bol { ye_le "ank"; }
        bahuroop kar inspect_type(x: patta[ank]): bol { ye_le "patta_ank"; }

        pakad my_var: ank = 123;
        pakad p_mut: patta[badal ank] = &my_var;
        pakad res: bol = inspect_type(p_mut);
    )raw";
    TEST_ASSERT(checkTypeOnly(mutPointerToConstParam), "TypeChecker: mutable pointer resolves to patta[ank] candidate", "Failed mut pointer dispatch");

    // 3. Return types preserved and strictly checked against target variables
    std::string returnTypeByInputType = R"raw(
        bahuroop kar transform(x: ank): ank { ye_le x * 2; }
        bahuroop kar transform(x: bol): bol { ye_le "str: " + x; }
        bahuroop kar transform(x: sach): sach { ye_le !x; }

        pakad out_num: ank = transform(10);
        pakad out_str: bol = transform("goat");
        pakad out_bool: sach = transform(na);
    )raw";
    TEST_ASSERT(checkTypeOnly(returnTypeByInputType), "TypeChecker: overload return types correctly propagate to assignments", "Failed return type propagation");

    // 4. Calling with unhandled type throws "Aukaat ke bahar!"
    std::string unhandledCall = R"raw(
        bahuroop kar only_primitives(x: ank): bol { ye_le "ank"; }
        bahuroop kar only_primitives(x: bol): bol { ye_le "bol"; }
        only_primitives([1, 2, 3]);
    )raw";
    std::string err;
    bool rejected = expectTypeError(unhandledCall, "Aukaat ke bahar! Jugaad 'only_primitives' ke liye koi matching overload nahi mila!", err);
    TEST_ASSERT(rejected, "TypeChecker: unhandled type rejected with 'Aukaat ke bahar'", "Error was: " + err);

    // 5. Calling with incompatible pointer pointee type throws "Aukaat ke bahar!"
    std::string incompatiblePointerCall = R"raw(
        bahuroop kar only_ank_ptr(x: patta[ank]): bol { ye_le "patta_ank"; }
        pakad s: bol = "test";
        pakad ps: patta[bol] = &s;
        only_ank_ptr(ps);
    )raw";
    std::string errPtr;
    bool rejectedPtr = expectTypeError(incompatiblePointerCall, "Aukaat ke bahar! Jugaad 'only_ank_ptr' ke liye koi matching overload nahi mila!", errPtr);
    TEST_ASSERT(rejectedPtr, "TypeChecker: patta[bol] rejected when candidate expects patta[ank]", "Error was: " + errPtr);
}

// ============================================================================
// SUITE 5: Ambiguous Overload Detection ("Confuse kar diya")
// ============================================================================
void testAmbiguousOverloadDetection()
{
    std::cout << "\n=== SUITE 5: Ambiguous Overload Detection ('Confuse kar diya') ===" << std::endl;

    // 1. Classical ambiguity: f(ank, any) vs f(any, ank) called with f(1, 2)
    std::string classicalAmbig = R"raw(
        bahuroop kar tie_fn(a: ank, b: any): ank { ye_le a; }
        bahuroop kar tie_fn(a: any, b: ank): ank { ye_le b; }
        tie_fn(1, 2);
    )raw";
    std::string err1;
    bool rej1 = expectTypeError(classicalAmbig, "Confuse kar diya! Jugaad 'tie_fn' ke overloads mein tie ho gaya!", err1);
    TEST_ASSERT(rej1, "TypeChecker: f(ank, any) vs f(any, ank) tie rejected with 'Confuse kar diya'", "Error was: " + err1);

    // 2. 3-parameter tie: f(ank, bol, any) vs f(ank, any, bol) called with f(1, "s", "t")
    std::string multiParamTie = R"raw(
        bahuroop kar trip_tie(a: ank, b: bol, c: any): ank { ye_le a; }
        bahuroop kar trip_tie(a: ank, b: any, c: bol): ank { ye_le a; }
        trip_tie(10, "hello", "world");
    )raw";
    std::string err2;
    bool rej2 = expectTypeError(multiParamTie, "Confuse kar diya! Jugaad 'trip_tie' ke overloads mein tie ho gaya!", err2);
    TEST_ASSERT(rej2, "TypeChecker: 3-parameter symmetric tie rejected with 'Confuse kar diya'", "Error was: " + err2);

    // 3. Exact duplicate signatures: score 0 vs score 0 tie
    std::string duplicateSig = R"raw(
        bahuroop kar dup_fn(x: ank, y: ank): ank { ye_le x + y; }
        bahuroop kar dup_fn(a: ank, b: ank): ank { ye_le a * b; }
        dup_fn(5, 10);
    )raw";
    std::string err3;
    bool rej3 = expectTypeError(duplicateSig, "Confuse kar diya! Jugaad 'dup_fn' ke overloads mein tie ho gaya!", err3);
    TEST_ASSERT(rej3, "TypeChecker: duplicate signatures tie rejected with 'Confuse kar diya'", "Error was: " + err3);
}

// ============================================================================
// SUITE 6: Subtype & Widening Overload Resolution
// ============================================================================
void testSubtypeAndWideningResolution()
{
    std::cout << "\n=== SUITE 6: Subtype & Widening Overload Resolution ===" << std::endl;

    // 1. Specific type (score 0) preferred over any (score 2)
    std::string specificOverAny = R"raw(
        bahuroop kar choose(x: any): bol { ye_le "any"; }
        bahuroop kar choose(x: ank): ank { ye_le 42; }

        pakad res: ank = choose(10);
    )raw";
    TEST_ASSERT(checkTypeOnly(specificOverAny), "TypeChecker: specific type candidate preferred over any", "Failed specific preference");

    // 2. String candidate (score 0) preferred over any (score 2)
    std::string stringOverAny = R"raw(
        bahuroop kar pick_str(x: any): ank { ye_le 0; }
        bahuroop kar pick_str(x: bol): bol { ye_le "string_chosen"; }

        pakad s_res: bol = pick_str("hello");
    )raw";
    TEST_ASSERT(checkTypeOnly(stringOverAny), "TypeChecker: bol candidate preferred over any", "Failed string preference");

    // 3. Numeric widening (score 1) preferred over any (score 2)
    std::string wideningOverAny = R"raw(
        bahuroop kar widen_pick(x: any): bol { ye_le "any"; }
        bahuroop kar widen_pick(x: point): point { ye_le 3.14; }

        pakad num_res: point = widen_pick(10);
    )raw";
    TEST_ASSERT(checkTypeOnly(wideningOverAny), "TypeChecker: numeric widening (point) preferred over any for integer argument", "Failed widening preference");

    // 4. Pointer subtyping preference: patta[ank] (score 1) preferred over any (score 2) when passing mutable pointer
    std::string ptrSubtypeOverAny = R"raw(
        bahuroop kar ptr_dispatch(x: any): ank { ye_le 0; }
        bahuroop kar ptr_dispatch(x: patta[ank]): bol { ye_le "ptr_chosen"; }

        pakad val: ank = 99;
        pakad p_mut: patta[badal ank] = &val;
        pakad ptr_res: bol = ptr_dispatch(p_mut);
    )raw";
    TEST_ASSERT(checkTypeOnly(ptrSubtypeOverAny), "TypeChecker: patta[ank] preferred over any for mutable pointer", "Failed pointer preference");
}

// ============================================================================
// SUITE 7: Address-Of Rvalue Rejection ("Doglapan")
// ============================================================================
void testAddressOfRvalueRejection()
{
    std::cout << "\n=== SUITE 7: Address-Of Rvalue Rejection ('Doglapan') ===" << std::endl;

    const std::string doglapan = "Doglapan! Sirf variable ya memory location ka patta liya ja sakta hai, temporary rvalue ka nahi!";

    // 1. Binary arithmetic rvalue
    std::string rval1 = "pakad p = &(1 + 2);";
    std::string err1;
    TEST_ASSERT(expectTypeError(rval1, doglapan, err1), "Address-of &(1 + 2) rejected", "Error was: " + err1);

    // 2. String literal rvalue
    std::string rval2 = "pakad p = &\"hello\";";
    std::string err2;
    TEST_ASSERT(expectTypeError(rval2, doglapan, err2), "Address-of &\"hello\" rejected", "Error was: " + err2);

    // 3. Boolean literal rvalue
    std::string rval3 = "pakad p = &han;";
    std::string err3;
    TEST_ASSERT(expectTypeError(rval3, doglapan, err3), "Address-of &han rejected", "Error was: " + err3);

    // 4. Array literal rvalue
    std::string rval4 = "pakad p = &[1, 2, 3];";
    std::string err4;
    TEST_ASSERT(expectTypeError(rval4, doglapan, err4), "Address-of &[1, 2, 3] rejected", "Error was: " + err4);

    // 5. Function call result rvalue
    std::string rval5 = "kar get_val(): ank { ye_le 10; } pakad p = &get_val();";
    std::string err5;
    TEST_ASSERT(expectTypeError(rval5, doglapan, err5), "Address-of &func() rejected", "Error was: " + err5);

    // 6. Desi keyword 'patta' with rvalue
    std::string rval6 = "pakad p = patta (100 + 200);";
    std::string err6;
    TEST_ASSERT(expectTypeError(rval6, doglapan, err6), "patta (100 + 200) rejected", "Error was: " + err6);

    // 7. Valid LValues MUST pass:
    // 7a. Variable identifier
    TEST_ASSERT(checkTypeOnly("pakad a = 10; pakad p = &a;"), "Valid LValue: &variable", "Failed &variable");
    // 7b. Array indexing
    TEST_ASSERT(checkTypeOnly("pakad arr = [10, 20]; pakad p = &arr[0];"), "Valid LValue: &array[index]", "Failed &array[index]");
    // 7c. Dereference of pointer (*p)
    TEST_ASSERT(checkTypeOnly("pakad a = 10; pakad p = &a; pakad p2 = &(*p);"), "Valid LValue: &(*ptr)", "Failed &(*ptr)");
}

// ============================================================================
// SUITE 8: Const Pointer & Const Variable Mutation Rejection
// ============================================================================
void testConstPointerMutationRejection()
{
    std::cout << "\n=== SUITE 8: Const Pointer & Const Variable Mutation Rejection ===" << std::endl;

    const std::string bheemConstPointer = "Shanti gadadhari Bheem! Const / atal pointer ke through memory mutate nahi kar sakte!";
    const std::string bheemConstVar = "Shanti gadadhari Bheem! Constant / atal variable 'SAMAJ' ko modify nahi kar sakte!";

    // 1. Mutating const pointer target (derived from atal variable)
    std::string mutateConstPointee1 = R"raw(
        pakad atal x: ank = 100;
        pakad p = &x;
        *p = 200;
    )raw";
    std::string err1;
    TEST_ASSERT(expectTypeError(mutateConstPointee1, bheemConstPointer, err1), "Mutating const pointer target from atal variable rejected", "Error was: " + err1);

    // 2. Mutating pointer explicitly typed as patta[ank] (immutable pointer)
    std::string mutateConstPointee2 = R"raw(
        pakad x: ank = 100;
        pakad p: patta[ank] = &x;
        *p = 200;
    )raw";
    std::string err2;
    TEST_ASSERT(expectTypeError(mutateConstPointee2, bheemConstPointer, err2), "Mutating explicitly typed patta[ank] pointer target rejected", "Error was: " + err2);

    // 3. Mutating using Desi keyword 'asli'
    std::string mutateWithAsli = R"raw(
        pakad atal x: ank = 100;
        pakad p = patta x;
        asli p = 500;
    )raw";
    std::string err3;
    TEST_ASSERT(expectTypeError(mutateWithAsli, bheemConstPointer, err3), "Mutating const pointer with 'asli p = ...' rejected", "Error was: " + err3);

    // 4. Directly reassigning atal variable rejected
    std::string reassignAtal = R"raw(
        pakad atal SAMAJ: ank = 42;
        SAMAJ = 99;
    )raw";
    std::string err4;
    TEST_ASSERT(expectTypeError(reassignAtal, bheemConstVar, err4), "Direct reassignment to atal variable rejected", "Error was: " + err4);

    // 5. Mutable pointer mutation IS permitted
    std::string validMutate = R"raw(
        pakad x: ank = 10;
        pakad p: patta[badal ank] = &x;
        *p = 20;
    )raw";
    TEST_ASSERT(checkTypeOnly(validMutate), "Mutating mutable pointer target *p = 20 permitted", "Failed valid mutation");

    // 6. Deref assignment type mismatch rejected with "Aukaat ke bahar!"
    std::string typeMismatchMutate = R"raw(
        pakad x: ank = 10;
        pakad p: patta[badal ank] = &x;
        *p = "text";
    )raw";
    std::string err6;
    bool rejMismatch = expectTypeError(typeMismatchMutate, "Aukaat ke bahar! Pointer type 'patta[badal ank]' mein 'bol' assign nahi kar sakte!", err6);
    TEST_ASSERT(rejMismatch, "Deref assignment type mismatch rejected with 'Aukaat ke bahar'", "Error was: " + err6);
}

// ============================================================================
// SUITE 9: Non-Overloaded Single Function Diagnostic Preservation
// ============================================================================
void testSingleFunctionDiagnosticPreservation()
{
    std::cout << "\n=== SUITE 9: Single Function Legacy Diagnostic Preservation ===" << std::endl;

    // Legacy diagnostic for single function parameter type mismatch
    std::string singleFn = R"raw(
        kar single_fn(n: ank) { ye_le n; }
        single_fn("galat");
    )raw";
    std::string err1;
    bool rej1 = expectTypeError(singleFn, "Doglapan! Jugaad parameter ko 'ank' chahiye tha, par tumne 'bol' pass kiya!", err1);
    TEST_ASSERT(rej1, "Legacy diagnostic preserved for single function argument mismatch", "Error was: " + err1);

    // Return type mismatch
    std::string badRet = R"raw(
        kar bad_return(): ank {
            ye_le "dhokha";
        }
    )raw";
    std::string err2;
    bool rej2 = expectTypeError(badRet, "Dhokha! Jugaad ne 'ank' wapas karne ka wada kiya tha, par 'bol' fenk diya!", err2);
    TEST_ASSERT(rej2, "Legacy diagnostic preserved for return type mismatch", "Error was: " + err2);
}

// ============================================================================
// MAIN RUNNER
// ============================================================================
int main()
{
    std::cout << "========================================================\n";
    std::cout << "🎯 CHALLENGER 1: M3 EMPIRICAL STRESS TEST SUITE\n";
    std::cout << "========================================================\n";

    testValueAndMemoryInvariants();
    testPointerTypeSubtypingAndAssignability();
    testArityBasedOverloading();
    testTypeBasedOverloading();
    testAmbiguousOverloadDetection();
    testSubtypeAndWideningResolution();
    testAddressOfRvalueRejection();
    testConstPointerMutationRejection();
    testSingleFunctionDiagnosticPreservation();

    std::cout << "\n========================================================\n";
    std::cout << "SUMMARY: " << testsPassed << " PASSED, " << testsFailed << " FAILED\n";
    std::cout << "========================================================\n";

    return (testsFailed == 0) ? 0 : 1;
}
