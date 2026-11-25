#pragma once

#include "MemeUiTypes.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace MemeUI {

inline bool isUtf8Continuation(unsigned char c) {
    return (c & 0xC0) == 0x80;
}

inline size_t utf8PrevChar(const std::string &s, size_t idx) {
    if (idx == 0 || s.empty()) return 0;
    size_t i = std::min(idx, s.size()) - 1;
    while (i > 0 && isUtf8Continuation(static_cast<unsigned char>(s[i]))) {
        --i;
    }
    return i;
}

inline size_t utf8NextChar(const std::string &s, size_t idx) {
    if (idx >= s.size()) return s.size();
    size_t i = idx + 1;
    while (i < s.size() && isUtf8Continuation(static_cast<unsigned char>(s[i]))) {
        ++i;
    }
    return i;
}

inline size_t utf8CodepointCount(const std::string &s) {
    size_t cnt = 0;
    for (unsigned char c : s) {
        if (!isUtf8Continuation(c)) ++cnt;
    }
    return cnt;
}

inline bool isWordChar(unsigned char c) {
    return std::isalnum(c) || c == '_' || c >= 0x80;
}

inline size_t utf8WordLeft(const std::string &s, size_t idx) {
    size_t i = std::min(idx, s.size());
    while (i > 0) {
        size_t prev = utf8PrevChar(s, i);
        unsigned char ch = static_cast<unsigned char>(s[prev]);
        if (!std::isspace(ch)) break;
        i = prev;
    }
    if (i > 0) {
        size_t prev = utf8PrevChar(s, i);
        bool word = isWordChar(static_cast<unsigned char>(s[prev]));
        while (i > 0) {
            size_t p = utf8PrevChar(s, i);
            unsigned char ch = static_cast<unsigned char>(s[p]);
            if (std::isspace(ch) || isWordChar(ch) != word) break;
            i = p;
        }
    }
    return i;
}

inline size_t utf8WordRight(const std::string &s, size_t idx) {
    size_t i = std::min(idx, s.size());
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
        i = utf8NextChar(s, i);
    }
    if (i < s.size()) {
        bool word = isWordChar(static_cast<unsigned char>(s[i]));
        while (i < s.size()) {
            unsigned char ch = static_cast<unsigned char>(s[i]);
            if (std::isspace(ch) || isWordChar(ch) != word) break;
            i = utf8NextChar(s, i);
        }
    }
    return i;
}

inline std::pair<size_t, size_t> utf8WordBoundsAt(const std::string &s, size_t idx) {
    if (s.empty()) return {0, 0};
    size_t i = std::min(idx, s.size());
    if (i == s.size() && i > 0) i = utf8PrevChar(s, i);
    unsigned char ch = static_cast<unsigned char>(s[i]);
    if (std::isspace(ch)) {
        size_t a = i, b = utf8NextChar(s, i);
        while (a > 0 && std::isspace(static_cast<unsigned char>(s[utf8PrevChar(s, a)]))) a = utf8PrevChar(s, a);
        while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) b = utf8NextChar(s, b);
        return {a, b};
    }
    bool word = isWordChar(ch);
    size_t a = i, b = utf8NextChar(s, i);
    while (a > 0) {
        size_t p = utf8PrevChar(s, a);
        unsigned char pc = static_cast<unsigned char>(s[p]);
        if (std::isspace(pc) || isWordChar(pc) != word) break;
        a = p;
    }
    while (b < s.size()) {
        unsigned char nc = static_cast<unsigned char>(s[b]);
        if (std::isspace(nc) || isWordChar(nc) != word) break;
        b = utf8NextChar(s, b);
    }
    return {a, b};
}

inline std::pair<size_t, size_t> utf8LineBoundsAt(const std::string &s, size_t idx) {
    if (s.empty()) return {0, 0};
    size_t i = std::min(idx, s.size());
    size_t a = i;
    while (a > 0 && s[a - 1] != '\n') --a;
    size_t b = i;
    while (b < s.size() && s[b] != '\n') ++b;
    if (b < s.size() && s[b] == '\n') ++b;
    return {a, b};
}

inline uint32_t decodeGlyphCodepoint(const std::string &text, size_t pos, size_t &nextPos) {
    nextPos = utf8NextChar(text, pos);
    size_t len = (nextPos > pos) ? (nextPos - pos) : 0;
    if (len == 0) return 0;
    unsigned char c0 = static_cast<unsigned char>(text[pos]);
    if (len == 1) {
        if (c0 == '\t') return ' ';
        if (c0 < 32 || c0 == 127) return 0;
        return c0;
    }
    if (len == 2 && c0 == 0xC2 && static_cast<unsigned char>(text[pos + 1]) == 0xA0) {
        return ' ';
    }
    if (len == 3 && c0 == 0xE2 && static_cast<unsigned char>(text[pos + 1]) == 0x80) {
        unsigned char c2 = static_cast<unsigned char>(text[pos + 2]);
        if (c2 == 0xA2) return 0x2022; // U+2022 bullet
        if (c2 == 0x93 || c2 == 0x94) return '-'; // en/em dash
        if (c2 == 0x98 || c2 == 0x99) return '\''; // single curly quote
        if (c2 == 0x9C || c2 == 0x9D) return '"'; // double curly quote
        if (c2 == 0xA6) return '.'; // ellipsis fallback
    }
    return '?';
}

inline float getCharAdvanceFactor(uint32_t cp, bool isMono = false) {
    if (cp == 0) return 0.0f;
    if (isMono) return 0.54f;
    switch (cp) {
    case ' ':  return 0.27f;
    case 'i': case 'l': case '!': case '|': case '.': case ',': case ':': case ';': case '\'': case '`':
        return 0.25f;
    case 'I':
        return 0.28f;
    case 'f': case 'j': case 't': case 'r': case '(': case ')': case '[': case ']': case '"':
        return 0.34f;
    case '-': case '/': case '\\':
        return 0.34f;
    case '{': case '}': case '*': case '?':
        return 0.42f;
    case 'J': case 'c': case 's': case 'z': case 'k': case 'v': case 'x': case 'y':
        return 0.47f;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
        return 0.53f;
    case 'a': case 'b': case 'd': case 'e': case 'g': case 'h': case 'n': case 'o': case 'p': case 'q': case 'u':
    case '+': case '=': case '<': case '>': case '^': case '_': case '~': case '$':
        return 0.53f;
    case 'E': case 'F': case 'L': case 'P': case 'S': case 'T': case 'Z':
        return 0.56f;
    case 'A': case 'B': case 'C': case 'D': case 'G': case 'H': case 'K': case 'N': case 'O': case 'Q': case 'R': case 'U': case 'V': case 'X': case 'Y':
    case '#': case '&':
        return 0.62f;
    case 'm': case 'w': case '%':
        return 0.74f;
    case 'M': case 'W': case '@':
        return 0.80f;
    case 0x2022:
        return 0.44f;
    default:
        return (cp >= 0xC0) ? 0.62f : 0.53f;
    }
}

inline float measureTextWidth(const std::string &text, float fontSize, bool isMono = false) {
    if (text.empty()) return 0.0f;
    float w = 0.0f;
    for (size_t i = 0; i < text.size();) {
        size_t next = i;
        uint32_t cp = decodeGlyphCodepoint(text, i, next);
        w += fontSize * getCharAdvanceFactor(cp, isMono);
        i = next;
    }
    return w;
}

inline std::string maskPasswordString(const std::string &s) {
    std::string masked;
    size_t n = utf8CodepointCount(s);
    masked.reserve(n * 3);
    for (size_t i = 0; i < n; ++i) {
        masked += "\xE2\x80\xA2"; // U+2022 bullet
    }
    return masked;
}

// Maps byte offset in plain text to byte offset in password-masked string
inline size_t mapPlainByteToDisplayByte(const std::string &plain, size_t plainByte, bool password) {
    if (!password) return std::min(plainByte, plain.size());
    size_t target = std::min(plainByte, plain.size());
    size_t cp = 0;
    for (size_t i = 0; i < target;) {
        i = utf8NextChar(plain, i);
        ++cp;
    }
    return cp * 3;
}

inline size_t mapDisplayByteToPlainByte(const std::string &plain, size_t dispByte, bool password) {
    if (!password) return std::min(dispByte, plain.size());
    size_t cpTarget = dispByte / 3;
    size_t i = 0;
    for (size_t cp = 0; cp < cpTarget && i < plain.size(); ++cp) {
        i = utf8NextChar(plain, i);
    }
    return i;
}

struct WrappedTextLine {
    size_t startByte = 0;
    size_t endByte = 0;
    std::string text;
    float width = 0.0f;
};

inline std::vector<WrappedTextLine> wrapTextLines(const std::string &text, float maxWidth, float fontSize, bool isMono = false) {
    std::vector<WrappedTextLine> lines;
    if (text.empty()) {
        lines.push_back({0, 0, "", 0.0f});
        return lines;
    }
    float effectiveW = std::max(24.0f, maxWidth);
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t nl = text.find('\n', pos);
        size_t segEnd = (nl == std::string::npos) ? text.size() : nl;

        if (segEnd == pos) {
            lines.push_back({pos, pos, "", 0.0f});
        } else {
            size_t lineStart = pos;
            while (lineStart < segEnd) {
                size_t cur = lineStart;
                size_t lastSpace = std::string::npos;
                float curW = 0.0f;
                while (cur < segEnd) {
                    size_t next = utf8NextChar(text, cur);
                    std::string ch = text.substr(cur, next - cur);
                    float cw = measureTextWidth(ch, fontSize, isMono);
                    if (cur > lineStart && curW + cw > effectiveW) {
                        break;
                    }
                    if (ch == " ") lastSpace = cur;
                    curW += cw;
                    cur = next;
                }
                if (cur < segEnd && lastSpace != std::string::npos && lastSpace > lineStart) {
                    std::string sub = text.substr(lineStart, lastSpace - lineStart);
                    lines.push_back({lineStart, lastSpace, sub, measureTextWidth(sub, fontSize, isMono)});
                    lineStart = lastSpace + 1;
                } else {
                    if (cur == lineStart) cur = utf8NextChar(text, lineStart);
                    std::string sub = text.substr(lineStart, cur - lineStart);
                    lines.push_back({lineStart, cur, sub, measureTextWidth(sub, fontSize, isMono)});
                    lineStart = cur;
                }
            }
        }

        if (nl == std::string::npos) break;
        pos = nl + 1;
        if (pos == text.size()) {
            lines.push_back({pos, pos, "", 0.0f});
            break;
        }
    }
    return lines;
}

struct TextUndoSnapshot {
    std::string text;
    size_t cursorByte = 0;
    size_t anchorByte = 0;
};

struct TextFieldState {
    std::string text;
    size_t cursorByte = 0;
    size_t anchorByte = 0;
    float scrollX = 0.0f;
    float scrollY = 0.0f;
    bool multiline = false;
    bool password = false;
    bool readOnly = false;
    bool disabled = false;
    bool dragging = false;
    std::string preedit;
    double blinkAccumMs = 0.0;
    bool caretVisible = true;
    int lastEditOpKind = 0; // 0=none, 1=insert, 2=delete

    std::vector<TextUndoSnapshot> undoStack;
    std::vector<TextUndoSnapshot> redoStack;

    void syncFromProp(const std::string &propText) {
        if (propText != text) {
            text = propText;
            cursorByte = std::min(cursorByte, text.size());
            anchorByte = std::min(anchorByte, text.size());
        }
    }

    bool hasSelection() const {
        return cursorByte != anchorByte;
    }

    std::pair<size_t, size_t> selectionRange() const {
        return {std::min(cursorByte, anchorByte), std::max(cursorByte, anchorByte)};
    }

    std::string selectedText() const {
        if (!hasSelection() || password) return "";
        auto [a, b] = selectionRange();
        return text.substr(a, b - a);
    }

    void pushUndoSnapshot(int opKind) {
        if (opKind != 0 && opKind == lastEditOpKind && !undoStack.empty()) {
            // Coalesce consecutive typing or backspaces
            return;
        }
        undoStack.push_back({text, cursorByte, anchorByte});
        if (undoStack.size() > 100) undoStack.erase(undoStack.begin());
        redoStack.clear();
        lastEditOpKind = opKind;
    }

    void resetBlink() {
        blinkAccumMs = 0.0;
        caretVisible = true;
    }

    void tickBlink(double deltaMs) {
        blinkAccumMs += deltaMs;
        if (blinkAccumMs >= 530.0) {
            blinkAccumMs = std::fmod(blinkAccumMs, 530.0);
            caretVisible = !caretVisible;
        }
    }

    bool undo() {
        if (readOnly || disabled || undoStack.empty()) return false;
        redoStack.push_back({text, cursorByte, anchorByte});
        auto snap = undoStack.back();
        undoStack.pop_back();
        text = snap.text;
        cursorByte = std::min(snap.cursorByte, text.size());
        anchorByte = std::min(snap.anchorByte, text.size());
        lastEditOpKind = 0;
        resetBlink();
        return true;
    }

    bool redo() {
        if (readOnly || disabled || redoStack.empty()) return false;
        undoStack.push_back({text, cursorByte, anchorByte});
        auto snap = redoStack.back();
        redoStack.pop_back();
        text = snap.text;
        cursorByte = std::min(snap.cursorByte, text.size());
        anchorByte = std::min(snap.anchorByte, text.size());
        lastEditOpKind = 0;
        resetBlink();
        return true;
    }

    void deleteSelectionInternal() {
        if (!hasSelection()) return;
        auto [a, b] = selectionRange();
        text.erase(a, b - a);
        cursorByte = a;
        anchorByte = a;
    }

    bool insertText(const std::string &ins) {
        if (readOnly || disabled || ins.empty()) return false;
        pushUndoSnapshot(1);
        deleteSelectionInternal();
        text.insert(cursorByte, ins);
        cursorByte += ins.size();
        anchorByte = cursorByte;
        resetBlink();
        return true;
    }

    bool backspace(bool byWord = false) {
        if (readOnly || disabled) return false;
        if (hasSelection()) {
            pushUndoSnapshot(0);
            deleteSelectionInternal();
            resetBlink();
            return true;
        }
        if (cursorByte == 0) return false;
        pushUndoSnapshot(byWord ? 0 : 2);
        size_t delStart = byWord ? utf8WordLeft(text, cursorByte) : utf8PrevChar(text, cursorByte);
        text.erase(delStart, cursorByte - delStart);
        cursorByte = delStart;
        anchorByte = cursorByte;
        resetBlink();
        return true;
    }

    bool delForward(bool byWord = false) {
        if (readOnly || disabled) return false;
        if (hasSelection()) {
            pushUndoSnapshot(0);
            deleteSelectionInternal();
            resetBlink();
            return true;
        }
        if (cursorByte >= text.size()) return false;
        pushUndoSnapshot(byWord ? 0 : 2);
        size_t delEnd = byWord ? utf8WordRight(text, cursorByte) : utf8NextChar(text, cursorByte);
        text.erase(cursorByte, delEnd - cursorByte);
        anchorByte = cursorByte;
        resetBlink();
        return true;
    }

    void selectAll() {
        anchorByte = 0;
        cursorByte = text.size();
        lastEditOpKind = 0;
        resetBlink();
    }

    void moveCursor(size_t newPos, bool keepSelection) {
        cursorByte = std::min(newPos, text.size());
        if (!keepSelection) anchorByte = cursorByte;
        lastEditOpKind = 0;
        resetBlink();
    }

    // Hit-test local (lx, ly) inside content box into byte offset
    size_t byteOffsetAtLocalPos(float lx, float ly, float boxW, float fontSize, bool isMono) const {
        std::string disp = password ? maskPasswordString(text) : text;
        if (!multiline) {
            float targetX = lx + scrollX;
            if (targetX <= 0.0f) return 0;
            size_t i = 0;
            float curX = 0.0f;
            while (i < disp.size()) {
                size_t next = utf8NextChar(disp, i);
                float cw = measureTextWidth(disp.substr(i, next - i), fontSize, isMono);
                if (targetX < curX + cw * 0.5f) {
                    return mapDisplayByteToPlainByte(text, i, password);
                }
                curX += cw;
                i = next;
            }
            return text.size();
        } else {
            auto lines = wrapTextLines(disp, boxW, fontSize, isMono);
            float lineH = fontSize * 1.35f;
            float targetY = ly + scrollY;
            int row = std::clamp(static_cast<int>(std::floor(targetY / lineH)), 0, static_cast<int>(lines.size()) - 1);
            const auto &ln = lines[row];
            float targetX = std::max(0.0f, lx);
            size_t i = 0;
            float curX = 0.0f;
            while (i < ln.text.size()) {
                size_t next = utf8NextChar(ln.text, i);
                float cw = measureTextWidth(ln.text.substr(i, next - i), fontSize, isMono);
                if (targetX < curX + cw * 0.5f) {
                    return mapDisplayByteToPlainByte(text, ln.startByte + i, password);
                }
                curX += cw;
                i = next;
            }
            return mapDisplayByteToPlainByte(text, ln.endByte, password);
        }
    }

    bool handlePointerDown(float lx, float ly, float boxW, float fontSize, bool isMono, int clickCount) {
        size_t pos = byteOffsetAtLocalPos(lx, ly, boxW, fontSize, isMono);
        if (clickCount >= 3) {
            if (multiline) {
                auto [a, b] = utf8LineBoundsAt(text, pos);
                anchorByte = a;
                cursorByte = b;
            } else {
                selectAll();
            }
        } else if (clickCount == 2 && !password) {
            auto [a, b] = utf8WordBoundsAt(text, pos);
            anchorByte = a;
            cursorByte = b;
        } else {
            cursorByte = pos;
            anchorByte = pos;
            dragging = true;
        }
        lastEditOpKind = 0;
        resetBlink();
        return true;
    }

    bool handlePointerDrag(float lx, float ly, float boxW, float fontSize, bool isMono) {
        if (!dragging) return false;
        cursorByte = byteOffsetAtLocalPos(lx, ly, boxW, fontSize, isMono);
        resetBlink();
        return true;
    }

    void handlePointerUp() {
        dragging = false;
    }

    struct KeyActionResult {
        bool textChanged = false;
        bool handled = false;
        bool submitEnter = false;
        std::string clipboardSet;
        bool requestClipboardCopy = false;
    };

    KeyActionResult handleKeyEvent(const UiKeyEvent &ev, float boxW, float boxH, float fontSize, bool isMono, const std::string &clipboardText) {
        KeyActionResult res;
        if (disabled) return res;

        bool primaryMod = ev.ctrl || ev.meta;
        bool wordJump = ev.alt || ev.ctrl;

        if (primaryMod && (ev.key == "a" || ev.key == "A")) {
            selectAll();
            res.handled = true;
            return res;
        }
        if (primaryMod && (ev.key == "c" || ev.key == "C")) {
            if (hasSelection() && !password) {
                res.clipboardSet = selectedText();
                res.requestClipboardCopy = true;
            }
            res.handled = true;
            return res;
        }
        if (primaryMod && (ev.key == "x" || ev.key == "X")) {
            if (hasSelection() && !password && !readOnly) {
                res.clipboardSet = selectedText();
                res.requestClipboardCopy = true;
                pushUndoSnapshot(0);
                deleteSelectionInternal();
                res.textChanged = true;
            }
            res.handled = true;
            return res;
        }
        if (primaryMod && (ev.key == "v" || ev.key == "V")) {
            if (!readOnly && !clipboardText.empty()) {
                std::string ins = clipboardText;
                if (!multiline) {
                    for (char &c : ins) {
                        if (c == '\n' || c == '\r') c = ' ';
                    }
                }
                res.textChanged = insertText(ins);
            }
            res.handled = true;
            return res;
        }
        if (primaryMod && (ev.key == "z" || ev.key == "Z")) {
            res.textChanged = ev.shift ? redo() : undo();
            res.handled = true;
            return res;
        }
        if (primaryMod && (ev.key == "y" || ev.key == "Y")) {
            res.textChanged = redo();
            res.handled = true;
            return res;
        }

        if (ev.key == "ArrowLeft") {
            size_t nextPos = (ev.meta && !ev.ctrl)
                ? utf8LineBoundsAt(text, cursorByte).first
                : (wordJump ? utf8WordLeft(text, cursorByte) : utf8PrevChar(text, cursorByte));
            moveCursor(nextPos, ev.shift);
            res.handled = true;
        } else if (ev.key == "ArrowRight") {
            size_t nextPos = (ev.meta && !ev.ctrl)
                ? utf8LineBoundsAt(text, cursorByte).second
                : (wordJump ? utf8WordRight(text, cursorByte) : utf8NextChar(text, cursorByte));
            if (ev.meta && !ev.ctrl && nextPos > 0 && nextPos <= text.size() && text[nextPos - 1] == '\n') {
                --nextPos;
            }
            moveCursor(nextPos, ev.shift);
            res.handled = true;
        } else if (ev.key == "Home") {
            size_t nextPos = multiline ? utf8LineBoundsAt(text, cursorByte).first : 0;
            moveCursor(nextPos, ev.shift);
            res.handled = true;
        } else if (ev.key == "End") {
            size_t nextPos = multiline ? utf8LineBoundsAt(text, cursorByte).second : text.size();
            if (multiline && nextPos > 0 && text[nextPos - 1] == '\n') --nextPos;
            moveCursor(nextPos, ev.shift);
            res.handled = true;
        } else if (multiline && (ev.key == "ArrowUp" || ev.key == "ArrowDown")) {
            std::string disp = password ? maskPasswordString(text) : text;
            auto lines = wrapTextLines(disp, boxW, fontSize, isMono);
            size_t dispCur = mapPlainByteToDisplayByte(text, cursorByte, password);
            int curRow = 0;
            float curX = 0.0f;
            for (int r = 0; r < static_cast<int>(lines.size()); ++r) {
                if (dispCur >= lines[r].startByte && dispCur <= lines[r].endByte) {
                    curRow = r;
                    curX = measureTextWidth(lines[r].text.substr(0, dispCur - lines[r].startByte), fontSize, isMono);
                    break;
                }
            }
            int nextRow = (ev.key == "ArrowUp") ? std::max(0, curRow - 1) : std::min(static_cast<int>(lines.size()) - 1, curRow + 1);
            float lineH = fontSize * 1.35f;
            size_t newPos = byteOffsetAtLocalPos(curX, nextRow * lineH + lineH * 0.5f - scrollY, boxW, fontSize, isMono);
            moveCursor(newPos, ev.shift);
            res.handled = true;
        } else if (ev.key == "Backspace") {
            res.textChanged = backspace(wordJump);
            res.handled = true;
        } else if (ev.key == "Delete") {
            res.textChanged = delForward(wordJump);
            res.handled = true;
        } else if (ev.key == "Enter") {
            if (multiline) {
                res.textChanged = insertText("\n");
            } else {
                res.submitEnter = true;
            }
            res.handled = true;
        } else if (!ev.text.empty() && !primaryMod) {
            res.textChanged = insertText(ev.text);
            res.handled = true;
        }

        ensureCaretVisible(boxW, boxH, fontSize, isMono);
        return res;
    }

    void ensureCaretVisible(float boxW, float boxH, float fontSize, bool isMono) {
        std::string disp = password ? maskPasswordString(text) : text;
        size_t dispCur = mapPlainByteToDisplayByte(text, cursorByte, password);
        if (!multiline) {
            float caretX = measureTextWidth(disp.substr(0, std::min(dispCur, disp.size())), fontSize, isMono);
            float usableW = std::max(16.0f, boxW - 4.0f);
            if (caretX - scrollX > usableW) scrollX = caretX - usableW;
            if (caretX - scrollX < 0.0f) scrollX = caretX;
            if (scrollX < 0.0f) scrollX = 0.0f;
        } else {
            auto lines = wrapTextLines(disp, boxW, fontSize, isMono);
            float lineH = fontSize * 1.35f;
            int curRow = 0;
            for (int r = 0; r < static_cast<int>(lines.size()); ++r) {
                if (dispCur >= lines[r].startByte && dispCur <= lines[r].endByte) {
                    curRow = r;
                    break;
                }
            }
            float caretY = curRow * lineH;
            if (caretY - scrollY + lineH > boxH) scrollY = caretY + lineH - boxH;
            if (caretY - scrollY < 0.0f) scrollY = caretY;
            if (scrollY < 0.0f) scrollY = 0.0f;
        }
    }

    void emitDrawCommands(
        std::vector<GpuDrawCmd> &cmds,
        float innerX, float innerY, float innerW, float innerH,
        float fontSize, int fontWeight, const ColorRgba &textColor,
        const std::string &placeholder, bool isFocused, bool isMono, int nodeId, int zIndex) const
    {
        GpuDrawCmd clipPush;
        clipPush.type = "PUSH_CLIP";
        clipPush.x = innerX - 2.0f;
        clipPush.y = innerY - 2.0f;
        clipPush.width = innerW + 4.0f;
        clipPush.height = innerH + 4.0f;
        clipPush.zIndex = zIndex;
        cmds.push_back(clipPush);

        std::string activeText = text;
        if (isFocused && !preedit.empty()) {
            activeText.insert(std::min(cursorByte, activeText.size()), preedit);
        }

        std::string disp = password ? maskPasswordString(activeText) : activeText;
        bool showPlaceholder = disp.empty() && !placeholder.empty();

        if (!multiline) {
            if (isFocused && hasSelection() && !disp.empty()) {
                auto [a, b] = selectionRange();
                size_t da = mapPlainByteToDisplayByte(text, a, password);
                size_t db = mapPlainByteToDisplayByte(text, b, password);
                float sx = innerX - scrollX + measureTextWidth(disp.substr(0, da), fontSize, isMono);
                float ex = innerX - scrollX + measureTextWidth(disp.substr(0, db), fontSize, isMono);
                GpuDrawCmd sel;
                sel.type = "DRAW_SELECTION_RECT";
                sel.x = sx;
                sel.y = innerY + 1.0f;
                sel.width = std::max(2.0f, ex - sx);
                sel.height = fontSize * 1.25f;
                sel.color = "#3b82f6";
                sel.alpha = 0.42f;
                sel.nodeId = nodeId;
                sel.zIndex = zIndex;
                cmds.push_back(sel);
            }

            std::string drawStr = showPlaceholder ? placeholder : disp;
            if (!drawStr.empty()) {
                GpuDrawCmd tcmd;
                tcmd.type = "DRAW_TEXT";
                tcmd.nodeId = 0;
                tcmd.x = innerX - (showPlaceholder ? 0.0f : scrollX);
                tcmd.y = innerY + fontSize;
                tcmd.text = drawStr;
                tcmd.fontSize = fontSize;
                tcmd.fontWeight = fontWeight;
                tcmd.isMono = isMono;
                tcmd.color = showPlaceholder ? "#64748b" : textColor.hex;
                tcmd.alpha = showPlaceholder ? 0.7f : textColor.a;
                tcmd.zIndex = zIndex;
                cmds.push_back(tcmd);
            }

            if (isFocused && caretVisible) {
                size_t dc = mapPlainByteToDisplayByte(text, cursorByte, password) + (preedit.empty() ? 0 : (password ? utf8CodepointCount(preedit) * 3 : preedit.size()));
                float cx = innerX - scrollX + measureTextWidth(disp.substr(0, std::min(dc, disp.size())), fontSize, isMono);
                GpuDrawCmd caret;
                caret.type = "DRAW_CARET";
                caret.x = cx;
                caret.y = innerY + 1.0f;
                caret.width = 1.8f;
                caret.height = fontSize * 1.22f;
                caret.color = "#f8fafc";
                caret.alpha = 1.0f;
                caret.nodeId = nodeId;
                caret.zIndex = zIndex;
                cmds.push_back(caret);
            }
        } else {
            float lineH = fontSize * 1.35f;
            if (showPlaceholder) {
                GpuDrawCmd tcmd;
                tcmd.type = "DRAW_TEXT";
                tcmd.x = innerX;
                tcmd.y = innerY + fontSize;
                tcmd.text = placeholder;
                tcmd.fontSize = fontSize;
                tcmd.fontWeight = fontWeight;
                tcmd.isMono = isMono;
                tcmd.color = "#64748b";
                tcmd.alpha = 0.7f;
                tcmd.zIndex = zIndex;
                cmds.push_back(tcmd);
            } else {
                auto lines = wrapTextLines(disp, innerW, fontSize, isMono);
                auto [sa, sb] = selectionRange();
                size_t da = mapPlainByteToDisplayByte(text, sa, password);
                size_t db = mapPlainByteToDisplayByte(text, sb, password);
                size_t dc = mapPlainByteToDisplayByte(text, cursorByte, password);

                for (size_t r = 0; r < lines.size(); ++r) {
                    float ly = innerY + r * lineH - scrollY;
                    if (ly + lineH < innerY || ly > innerY + innerH) continue;

                    const auto &ln = lines[r];
                    if (isFocused && hasSelection() && db > ln.startByte && da < ln.endByte) {
                        size_t la = std::max(da, ln.startByte) - ln.startByte;
                        size_t lb = std::min(db, ln.endByte) - ln.startByte;
                        float sx = innerX + measureTextWidth(ln.text.substr(0, la), fontSize, isMono);
                        float ex = innerX + measureTextWidth(ln.text.substr(0, lb), fontSize, isMono);
                        GpuDrawCmd sel;
                        sel.type = "DRAW_SELECTION_RECT";
                        sel.x = sx;
                        sel.y = ly + 1.0f;
                        sel.width = std::max(3.0f, ex - sx);
                        sel.height = lineH * 0.9f;
                        sel.color = "#3b82f6";
                        sel.alpha = 0.42f;
                        sel.nodeId = nodeId;
                        sel.zIndex = zIndex;
                        cmds.push_back(sel);
                    }

                    if (!ln.text.empty()) {
                        GpuDrawCmd tcmd;
                        tcmd.type = "DRAW_TEXT";
                        tcmd.x = innerX;
                        tcmd.y = ly + fontSize;
                        tcmd.text = ln.text;
                        tcmd.fontSize = fontSize;
                        tcmd.fontWeight = fontWeight;
                        tcmd.isMono = isMono;
                        tcmd.color = textColor.hex;
                        tcmd.alpha = textColor.a;
                        tcmd.zIndex = zIndex;
                        cmds.push_back(tcmd);
                    }

                    if (isFocused && caretVisible && dc >= ln.startByte && dc <= ln.endByte) {
                        float cx = innerX + measureTextWidth(ln.text.substr(0, dc - ln.startByte), fontSize, isMono);
                        GpuDrawCmd caret;
                        caret.type = "DRAW_CARET";
                        caret.x = cx;
                        caret.y = ly + 1.0f;
                        caret.width = 1.8f;
                        caret.height = fontSize * 1.22f;
                        caret.color = "#f8fafc";
                        caret.alpha = 1.0f;
                        caret.nodeId = nodeId;
                        caret.zIndex = zIndex;
                        cmds.push_back(caret);
                    }
                }
            }
        }

        GpuDrawCmd clipPop;
        clipPop.type = "POP_CLIP";
        clipPop.zIndex = zIndex;
        cmds.push_back(clipPop);
    }
};

} // namespace MemeUI
