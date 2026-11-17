#!/usr/bin/env bash
# ==============================================================================
# MEMElang (OOP Edition) - Diagnostic Meme Error Catalog Test Runner
# Displays all iconic Indian meme error messages live using ./meme_oop
# ==============================================================================

# ANSI Color Codes
BOLD="\033[1m"
RESET="\033[0m"
CYAN="\033[36m"
YELLOW="\033[33m"
GREEN="\033[32m"
RED="\033[31m"
MAGENTA="\033[35m"
BLUE="\033[34m"

# Ensure meme_oop is built
if [ ! -f "./meme_oop" ]; then
    echo -e "${YELLOW}Compiling meme_oop.cpp...${RESET}"
    clang++ -std=c++17 -Wall -Wextra meme_oop.cpp -o meme_oop || {
        echo -e "${RED}Compilation failed!${RESET}"
        exit 1
    }
fi

echo -e "\n${BOLD}${MAGENTA}==============================================================================${RESET}"
echo -e "${BOLD}${MAGENTA}    ERROR SHOWCASE${RESET}"
echo -e "${BOLD}${MAGENTA}==============================================================================${RESET}"
echo -e "Running all diagnostic error scenarios through ${BOLD}./meme_oop${RESET}...\n"

COUNT=0

run_error_test() {
    COUNT=$((COUNT + 1))
    local code="$1"

    echo -e "${YELLOW}Code snippet:${RESET} ${BOLD}${code}${RESET}"
    echo -e "${BOLD}Output:${RESET}"
    ./meme_oop -e "$code" 2>&1
    echo -e "${MAGENTA}------------------------------------------------------------------------------${RESET}\n"
}

# ------------------------------------------------------------------------------
# 1. ARITHMETIC & OPERATOR ERRORS
# ------------------------------------------------------------------------------
run_error_test \
    "let a = 10 / 0;"

run_error_test \
    "let x = 15 % 0;"

run_error_test \
    "for i in 1 to 5 step 0 { print(i); }"

run_error_test \
    "let s = 'hello' * -3;"

run_error_test \
    "let neg = -'hello';"

# ------------------------------------------------------------------------------
# 2. TYPE MISMATCH ERRORS ("SEB KO SANTRE SE MAT TOLO")
# ------------------------------------------------------------------------------
run_error_test \
    "let res = 10 / 'Hello';"

run_error_test \
    "let res = 10 - 'Hello';"

run_error_test \
    "let res = true + false;"

run_error_test \
    "if (10 < 'hello') { print('Impossible'); }"

run_error_test \
    "let m = 10 % 'hello';"

# ------------------------------------------------------------------------------
# 3. SYNTAX, LEXER & PARSER ERRORS
# ------------------------------------------------------------------------------
run_error_test \
    "let a = 1 let b = 2;"

run_error_test \
    "let x = 10; (x) = 20;"

run_error_test \
    "let res = true * false;"

run_error_test \
    "let x = 5; if (x = 10) { print('Oops'); }"

run_error_test \
     "let x = 5; while (x = 10) { print('Oops'); }"

run_error_test \
    "let s = \"khula reh gaya;"

run_error_test \
    "print(1 + (2 * 3;"

run_error_test \
    "let x = @123;"

run_error_test \
    "let x = 10 +"

run_error_test \
    "let . = 5;"

# ------------------------------------------------------------------------------
# 4. VARIABLE & SCOPING RUNTIME ERRORS
# ------------------------------------------------------------------------------
run_error_test \
    "print(rasode_ki_rashi);"

run_error_test \
    "chuna = 500;"

# ------------------------------------------------------------------------------
# 5. FUNCTIONS & OOP ERRORS
# ------------------------------------------------------------------------------
run_error_test \
    "let x = 42; x();"

run_error_test \
    "class Influencer {} let i = Influencer(); print(i.humble);"

run_error_test \
    "let n = null; print(n.missing_property);"

run_error_test \
    "Fake Nepotism (Inheriting From Non-Class)" \
    "Bollywood Nepotism Debate" \
    "let starkid = 42; class StarKid extends starkid {}"

run_error_test \
     "function jod(a, b) { return a + b; } jod(10);"

# ------------------------------------------------------------------------------
# 6. MISPLACED CONTROL FLOW SIGNALS
# ------------------------------------------------------------------------------
run_error_test \
    "break;"

run_error_test \
    "continue;"

run_error_test \
    "return 42;"

# ------------------------------------------------------------------------------
# 7. FILE NOT FOUND ERROR
# ------------------------------------------------------------------------------
echo -e "${BOLD}${CYAN}[Error #$((COUNT + 1))] File Gayab Hai (Non-Existent File)${RESET}"
echo -e "${BLUE}Cultural Reference:${RESET} Main Isse Out Hoon (Shark Tank India)"
echo -e "${YELLOW}Command:${RESET} ${BOLD}./meme_oop nonexistent_script.meme${RESET}"
echo -e "${BOLD}Output:${RESET}"
./meme_oop nonexistent_script.meme 2>&1
echo -e "${MAGENTA}==============================================================================${RESET}\n"

echo -e "${BOLD}${GREEN}✅ All $COUNT error scenarios successfully displayed!${RESET}\n"
