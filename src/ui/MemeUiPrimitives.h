#pragma once

#include "MemeUiTypes.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cmath>
#include <functional>

namespace MemeUI {

// ---------------------------------------------------------------------------
// 1. UTF-8 Diacritic Folding & Normalization (for Typeahead)
// ---------------------------------------------------------------------------
inline std::string foldDiacriticsLower(const std::string &in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size();) {
        unsigned char c = static_cast<unsigned char>(in[i]);
        if (c < 0x80) {
            out.push_back(static_cast<char>(std::tolower(c)));
            ++i;
        } else if (c == 0xC3 && i + 1 < in.size()) {
            unsigned char c2 = static_cast<unsigned char>(in[i + 1]);
            char rep = '?';
            if ((c2 >= 0x80 && c2 <= 0x85) || (c2 >= 0xA0 && c2 <= 0xA5)) rep = 'a';
            else if (c2 == 0x87 || c2 == 0xA7) rep = 'c';
            else if ((c2 >= 0x88 && c2 <= 0x8B) || (c2 >= 0xA8 && c2 <= 0xAB)) rep = 'e';
            else if ((c2 >= 0x8C && c2 <= 0x8F) || (c2 >= 0xAC && c2 <= 0xAF)) rep = 'i';
            else if (c2 == 0x91 || c2 == 0xB1) rep = 'n';
            else if ((c2 >= 0x92 && c2 <= 0x96) || c2 == 0x98 || (c2 >= 0xB2 && c2 <= 0xB6) || c2 == 0xB8) rep = 'o';
            else if ((c2 >= 0x99 && c2 <= 0x9C) || (c2 >= 0xB9 && c2 <= 0xBC)) rep = 'u';
            else if (c2 == 0x9D || c2 == 0xBD || c2 == 0xBF) rep = 'y';
            out.push_back(rep);
            i += 2;
        } else {
            out.push_back(static_cast<char>(c));
            ++i;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// 2. Popup Primitive + Nested Popup Stack & Viewport Collision Solver
// ---------------------------------------------------------------------------
enum class PopupSide { TOP, BOTTOM, LEFT, RIGHT };
enum class PopupAlign { START, CENTER, END };

struct PopupPlacementOptions {
    PopupSide side = PopupSide::BOTTOM;
    PopupAlign align = PopupAlign::START;
    float sideOffset = 6.0f;
    float alignOffset = 0.0f;
    float viewportPadding = 8.0f;
    bool avoidCollisions = true;
    bool flip = true;
    bool shift = true;
    bool rtl = false;
};

struct PopupPlacementResult {
    float x = 0.0f;
    float y = 0.0f;
    float availableWidth = 0.0f;
    float availableHeight = 0.0f;
    PopupSide actualSide = PopupSide::BOTTOM;
    PopupAlign actualAlign = PopupAlign::START;
    bool flipped = false;
    bool shifted = false;
};

inline PopupPlacementResult computePopupPlacement(
    const UiRect &anchor,
    float popupW,
    float popupH,
    const UiRect &viewport,
    const PopupPlacementOptions &opts = {})
{
    PopupPlacementResult res;
    PopupSide side = opts.side;
    PopupAlign align = opts.align;
    if (opts.rtl) {
        if (side == PopupSide::LEFT) side = PopupSide::RIGHT;
        else if (side == PopupSide::RIGHT) side = PopupSide::LEFT;
    }

    // RTL flip for logical START/END when top/bottom, and LEFT/RIGHT swap if needed
    auto computeXY = [&](PopupSide s, PopupAlign al) -> std::pair<float, float> {
        float px = anchor.x;
        float py = anchor.y;
        bool horizAlign = (s == PopupSide::TOP || s == PopupSide::BOTTOM);
        PopupAlign effAlign = al;
        if (horizAlign && opts.rtl) {
            if (al == PopupAlign::START) effAlign = PopupAlign::END;
            else if (al == PopupAlign::END) effAlign = PopupAlign::START;
        }

        if (s == PopupSide::BOTTOM) py = anchor.y + anchor.height + opts.sideOffset;
        else if (s == PopupSide::TOP) py = anchor.y - popupH - opts.sideOffset;
        else if (s == PopupSide::RIGHT) px = anchor.x + anchor.width + opts.sideOffset;
        else if (s == PopupSide::LEFT) px = anchor.x - popupW - opts.sideOffset;

        if (horizAlign) {
            if (effAlign == PopupAlign::START) px = anchor.x + opts.alignOffset;
            else if (effAlign == PopupAlign::CENTER) px = anchor.x + (anchor.width - popupW) * 0.5f + opts.alignOffset;
            else px = anchor.x + anchor.width - popupW - opts.alignOffset;
        } else {
            if (effAlign == PopupAlign::START) py = anchor.y + opts.alignOffset;
            else if (effAlign == PopupAlign::CENTER) py = anchor.y + (anchor.height - popupH) * 0.5f + opts.alignOffset;
            else py = anchor.y + anchor.height - popupH - opts.alignOffset;
        }
        return {px, py};
    };

    auto [px, py] = computeXY(side, align);

    float minX = viewport.x + opts.viewportPadding;
    float minY = viewport.y + opts.viewportPadding;
    float maxX = viewport.x + viewport.width - opts.viewportPadding;
    float maxY = viewport.y + viewport.height - opts.viewportPadding;

    if (opts.avoidCollisions && opts.flip) {
        if (side == PopupSide::BOTTOM && py + popupH > maxY) {
            float spaceAbove = anchor.y - minY - opts.sideOffset;
            float spaceBelow = maxY - (anchor.y + anchor.height + opts.sideOffset);
            if (spaceAbove > spaceBelow && spaceAbove >= std::min(popupH, 80.0f)) {
                side = PopupSide::TOP;
                res.flipped = true;
            }
        } else if (side == PopupSide::TOP && py < minY) {
            float spaceAbove = anchor.y - minY - opts.sideOffset;
            float spaceBelow = maxY - (anchor.y + anchor.height + opts.sideOffset);
            if (spaceBelow > spaceAbove && spaceBelow >= std::min(popupH, 80.0f)) {
                side = PopupSide::BOTTOM;
                res.flipped = true;
            }
        } else if (side == PopupSide::RIGHT && px + popupW > maxX) {
            float spaceLeft = anchor.x - minX - opts.sideOffset;
            float spaceRight = maxX - (anchor.x + anchor.width + opts.sideOffset);
            if (spaceLeft > spaceRight) {
                side = PopupSide::LEFT;
                res.flipped = true;
            }
        } else if (side == PopupSide::LEFT && px < minX) {
            float spaceLeft = anchor.x - minX - opts.sideOffset;
            float spaceRight = maxX - (anchor.x + anchor.width + opts.sideOffset);
            if (spaceRight > spaceLeft) {
                side = PopupSide::RIGHT;
                res.flipped = true;
            }
        }
        if (res.flipped) {
            auto xy = computeXY(side, align);
            px = xy.first;
            py = xy.second;
        }
    }

    if (opts.avoidCollisions && opts.shift) {
        float origX = px, origY = py;
        if (popupW <= (maxX - minX)) {
            px = std::clamp(px, minX, maxX - popupW);
        } else {
            px = minX;
        }
        if (popupH <= (maxY - minY)) {
            py = std::clamp(py, minY, maxY - popupH);
        } else {
            py = minY;
        }
        if (std::abs(px - origX) > 0.01f || std::abs(py - origY) > 0.01f) {
            res.shifted = true;
        }
    }

    res.x = px;
    res.y = py;
    res.actualSide = side;
    res.actualAlign = align;
    res.availableWidth = std::max(0.0f, maxX - px);
    res.availableHeight = std::max(0.0f, (side == PopupSide::TOP) ? (anchor.y - opts.sideOffset - minY) : (maxY - py));
    return res;
}

struct PopupEntry {
    std::string id;
    std::string triggerId;
    std::string restoreFocusId;
    bool closeOnEscape = true;
    bool closeOnOutsideClick = true;
    bool modal = false;
    UiRect bounds = {0, 0, 0, 0};
    UiRect triggerBounds = {0, 0, 0, 0};
    std::function<void()> onDismiss = nullptr;
};

class PopupStack {
public:
    std::vector<PopupEntry> stack;

    void open(const PopupEntry &entry) {
        for (auto &e : stack) {
            if (e.id == entry.id) {
                e = entry;
                return;
            }
        }
        stack.push_back(entry);
    }

    bool isOpen(const std::string &id) const {
        for (const auto &e : stack) {
            if (e.id == id) return true;
        }
        return false;
    }

    std::string closeTop() {
        if (stack.empty()) return "";
        PopupEntry top = stack.back();
        stack.pop_back();
        if (top.onDismiss) top.onDismiss();
        return top.restoreFocusId;
    }

    std::string closeById(const std::string &id) {
        for (size_t i = 0; i < stack.size(); ++i) {
            if (stack[i].id == id) {
                std::string restore = stack[i].restoreFocusId;
                // Closing parent also closes any nested child popups above it!
                std::vector<PopupEntry> toClose(stack.begin() + i, stack.end());
                stack.erase(stack.begin() + i, stack.end());
                for (auto it = toClose.rbegin(); it != toClose.rend(); ++it) {
                    if (it->onDismiss) it->onDismiss();
                }
                return restore;
            }
        }
        return "";
    }

    // Escape closes ONLY the topmost popup that allows closeOnEscape
    bool handleEscape(std::string &outRestoreFocusId) {
        if (stack.empty()) return false;
        if (!stack.back().closeOnEscape) return true; // intercepted by non-dismissible modal
        outRestoreFocusId = closeTop();
        return true;
    }

    // Outside pointerdown dismisses top popups whose bounds (and trigger if child portal) do not contain (px, py)
    bool handlePointerDown(float px, float py, std::string &outRestoreFocusId) {
        if (stack.empty()) return false;
        bool dismissedAny = false;
        while (!stack.empty()) {
            const auto &top = stack.back();
            if (top.bounds.contains(px, py) || top.triggerBounds.contains(px, py)) {
                break;
            }
            if (!top.closeOnOutsideClick) {
                if (top.modal) break; // modal blocks clicks behind it
                break;
            }
            outRestoreFocusId = closeTop();
            dismissedAny = true;
        }
        return dismissedAny;
    }

    void clear() { stack.clear(); }
};

// ---------------------------------------------------------------------------
// 3. Roving Focus Primitive (vertical / horizontal / 2D grid / RTL / disabled)
// ---------------------------------------------------------------------------
struct RovingItem {
    std::string id;
    bool disabled = false;
};

class RovingFocusState {
public:
    std::vector<RovingItem> items;
    std::string activeId = "";
    std::string orientation = "vertical"; // "vertical", "horizontal", "grid"
    int columns = 1;
    bool loop = true;
    bool rtl = false;

    int indexOf(const std::string &id) const {
        for (size_t i = 0; i < items.size(); ++i) {
            if (items[i].id == id) return static_cast<int>(i);
        }
        return -1;
    }

    int firstEnabledIndex() const {
        for (size_t i = 0; i < items.size(); ++i) {
            if (!items[i].disabled) return static_cast<int>(i);
        }
        return -1;
    }

    int lastEnabledIndex() const {
        for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
            if (!items[i].disabled) return i;
        }
        return -1;
    }

    void ensureValidActive() {
        int idx = indexOf(activeId);
        if (idx >= 0 && !items[idx].disabled) return;
        int first = firstEnabledIndex();
        activeId = (first >= 0) ? items[first].id : "";
    }

    int stepFrom(int startIdx, int delta) const {
        int n = static_cast<int>(items.size());
        if (n == 0 || firstEnabledIndex() < 0) return -1;
        int cur = startIdx;
        for (int tried = 0; tried < n; ++tried) {
            int next = cur + delta;
            if (loop) {
                next = ((next % n) + n) % n;
            } else {
                if (next < 0 || next >= n) return -1;
            }
            if (!items[next].disabled) return next;
            cur = next;
        }
        return -1;
    }

    bool handleKey(const std::string &key, std::string &outFocusedId) {
        ensureValidActive();
        int n = static_cast<int>(items.size());
        if (n == 0 || firstEnabledIndex() < 0) return false;
        int cur = indexOf(activeId);
        if (cur < 0) cur = firstEnabledIndex();

        int nextIdx = -1;
        if (key == "Home") {
            nextIdx = firstEnabledIndex();
        } else if (key == "End") {
            nextIdx = lastEnabledIndex();
        } else if (orientation == "grid" || (orientation == "both" && columns > 1)) {
            int cols = std::max(1, columns);
            if (key == "ArrowRight") nextIdx = stepFrom(cur, rtl ? -1 : 1);
            else if (key == "ArrowLeft") nextIdx = stepFrom(cur, rtl ? 1 : -1);
            else if (key == "ArrowDown") nextIdx = stepFrom(cur, cols);
            else if (key == "ArrowUp") nextIdx = stepFrom(cur, -cols);
        } else if (orientation == "horizontal") {
            if (key == "ArrowRight") nextIdx = stepFrom(cur, rtl ? -1 : 1);
            else if (key == "ArrowLeft") nextIdx = stepFrom(cur, rtl ? 1 : -1);
        } else if (orientation == "both") {
            if (key == "ArrowDown") nextIdx = stepFrom(cur, 1);
            else if (key == "ArrowUp") nextIdx = stepFrom(cur, -1);
            else if (key == "ArrowRight") nextIdx = stepFrom(cur, rtl ? -1 : 1);
            else if (key == "ArrowLeft") nextIdx = stepFrom(cur, rtl ? 1 : -1);
        } else {
            if (key == "ArrowDown") nextIdx = stepFrom(cur, 1);
            else if (key == "ArrowUp") nextIdx = stepFrom(cur, -1);
        }

        if (nextIdx >= 0 && nextIdx < n) {
            activeId = items[nextIdx].id;
            outFocusedId = activeId;
            return true;
        }
        return false;
    }

    int getTabIndex(const std::string &id) const {
        if (activeId.empty()) {
            int f = firstEnabledIndex();
            return (f >= 0 && items[f].id == id) ? 0 : -1;
        }
        return (activeId == id) ? 0 : -1;
    }
};

// ---------------------------------------------------------------------------
// 4. Focus Trap / Focus Scope Stack (nested modals, Tab/Shift+Tab wrap, restore)
// ---------------------------------------------------------------------------
struct FocusScope {
    std::string id;
    std::string containerId;
    std::vector<std::string> focusableIds;
    std::string restoreFocusToId = "";
    std::string initialFocusId = "";
    bool trapped = true;
    bool paused = false;
};

class FocusScopeStack {
public:
    std::vector<FocusScope> scopes;

    std::string mountScope(const FocusScope &scope, const std::string &currentFocusedId) {
        for (auto &s : scopes) s.paused = true;
        FocusScope copy = scope;
        if (copy.restoreFocusToId.empty()) copy.restoreFocusToId = currentFocusedId;
        copy.paused = false;
        scopes.push_back(copy);
        if (!copy.initialFocusId.empty()) return copy.initialFocusId;
        if (!copy.focusableIds.empty()) return copy.focusableIds.front();
        return copy.containerId;
    }

    std::string unmountScope(const std::string &scopeId) {
        for (size_t i = 0; i < scopes.size(); ++i) {
            if (scopes[i].id == scopeId) {
                std::string restore = scopes[i].restoreFocusToId;
                scopes.erase(scopes.begin() + i);
                if (!scopes.empty()) {
                    scopes.back().paused = false;
                }
                return restore;
            }
        }
        return "";
    }

    const FocusScope *activeTrap() const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (!it->paused && it->trapped) return &(*it);
        }
        return nullptr;
    }

    bool handleTab(bool shift, const std::string &currentFocusedId, std::string &outNextFocusId) const {
        const FocusScope *trap = activeTrap();
        if (!trap) return false;
        const auto &ids = trap->focusableIds;
        if (ids.empty()) {
            outNextFocusId = trap->containerId;
            return true;
        }
        if (ids.size() == 1) {
            outNextFocusId = ids[0];
            return true;
        }
        int cur = -1;
        for (size_t i = 0; i < ids.size(); ++i) {
            if (ids[i] == currentFocusedId) { cur = static_cast<int>(i); break; }
        }
        if (cur < 0) {
            outNextFocusId = shift ? ids.back() : ids.front();
        } else if (shift) {
            outNextFocusId = ids[(cur - 1 + static_cast<int>(ids.size())) % static_cast<int>(ids.size())];
        } else {
            outNextFocusId = ids[(cur + 1) % static_cast<int>(ids.size())];
        }
        return true;
    }
};

// ---------------------------------------------------------------------------
// 5. Selection Primitive (single / multiple / range shift-click / selectAll)
// ---------------------------------------------------------------------------
class SelectionState {
public:
    std::string mode = "single"; // "single", "multiple", "none"
    bool disallowEmpty = false;
    std::vector<std::string> selectedIds;
    std::string anchorId = "";
    std::unordered_set<std::string> disabledIds;

    bool isSelected(const std::string &id) const {
        return std::find(selectedIds.begin(), selectedIds.end(), id) != selectedIds.end();
    }

    bool select(const std::string &id) {
        if (mode == "none" || disabledIds.count(id)) return false;
        if (mode == "single") {
            if (selectedIds.size() == 1 && selectedIds[0] == id) return false;
            selectedIds = {id};
            anchorId = id;
            return true;
        }
        if (!isSelected(id)) {
            selectedIds.push_back(id);
            anchorId = id;
            return true;
        }
        return false;
    }

    bool deselect(const std::string &id) {
        if (mode == "none" || disabledIds.count(id)) return false;
        if (disallowEmpty && selectedIds.size() <= 1 && isSelected(id)) return false;
        auto it = std::find(selectedIds.begin(), selectedIds.end(), id);
        if (it == selectedIds.end()) return false;
        selectedIds.erase(it);
        return true;
    }

    bool toggle(const std::string &id) {
        if (mode == "none" || disabledIds.count(id)) return false;
        if (isSelected(id)) {
            if (mode == "single" && disallowEmpty) return false;
            return deselect(id);
        }
        return select(id);
    }

    bool selectRange(const std::string &toId, const std::vector<std::string> &orderedIds) {
        if (mode != "multiple" || disabledIds.count(toId)) return false;
        if (anchorId.empty()) return select(toId);
        int idxA = -1, idxB = -1;
        for (int i = 0; i < static_cast<int>(orderedIds.size()); ++i) {
            if (orderedIds[i] == anchorId) idxA = i;
            if (orderedIds[i] == toId) idxB = i;
        }
        if (idxA < 0 || idxB < 0) return select(toId);
        int lo = std::min(idxA, idxB);
        int hi = std::max(idxA, idxB);
        selectedIds.clear();
        for (int i = lo; i <= hi; ++i) {
            if (!disabledIds.count(orderedIds[i])) {
                selectedIds.push_back(orderedIds[i]);
            }
        }
        return true;
    }

    bool selectAll(const std::vector<std::string> &orderedIds) {
        if (mode != "multiple") return false;
        selectedIds.clear();
        for (const auto &id : orderedIds) {
            if (!disabledIds.count(id)) selectedIds.push_back(id);
        }
        return true;
    }

    bool clear() {
        if (disallowEmpty && !selectedIds.empty()) return false;
        if (selectedIds.empty()) return false;
        selectedIds.clear();
        anchorId.clear();
        return true;
    }
};

// ---------------------------------------------------------------------------
// 6. Typeahead Primitive (timeout reset, prefix cycling, diacritic folding)
// ---------------------------------------------------------------------------
struct TypeaheadItem {
    std::string id;
    std::string text;
    bool disabled = false;
};

class TypeaheadState {
public:
    std::string buffer = "";
    double lastKeyTimeMs = -1e9;
    double timeoutMs = 500.0;

    void reset() {
        buffer.clear();
    }

    // Returns matched item ID or "" if no match
    std::string handleChar(
        const std::string &ch,
        double nowMs,
        const std::vector<TypeaheadItem> &items,
        const std::string &currentId = "")
    {
        if (ch.empty()) return "";
        if (ch == " " && buffer.empty()) return ""; // Space ignored unless continuing multi-char search

        if (nowMs - lastKeyTimeMs > timeoutMs) {
            buffer.clear();
        }
        lastKeyTimeMs = nowMs;
        std::string normCh = foldDiacriticsLower(ch);
        buffer += normCh;

        // Check if buffer is all repeated occurrences of the same character (e.g. "aaa") -> cycle!
        bool allSame = buffer.size() > 1;
        for (size_t i = 1; i < buffer.size(); ++i) {
            if (buffer[i] != buffer[0]) { allSame = false; break; }
        }

        std::string searchPrefix = allSame ? buffer.substr(0, 1) : buffer;

        int startIdx = 0;
        if (allSame && !currentId.empty()) {
            for (int i = 0; i < static_cast<int>(items.size()); ++i) {
                if (items[i].id == currentId) {
                    startIdx = (i + 1) % std::max(1, static_cast<int>(items.size()));
                    break;
                }
            }
        } else if (buffer.size() == 1 && !currentId.empty()) {
            // Single letter starts search after current item so repeated presses advance
            for (int i = 0; i < static_cast<int>(items.size()); ++i) {
                if (items[i].id == currentId) {
                    startIdx = (i + 1) % std::max(1, static_cast<int>(items.size()));
                    break;
                }
            }
        }

        int n = static_cast<int>(items.size());
        for (int k = 0; k < n; ++k) {
            int idx = (startIdx + k) % n;
            if (items[idx].disabled) continue;
            std::string itemNorm = foldDiacriticsLower(items[idx].text);
            if (itemNorm.rfind(searchPrefix, 0) == 0) {
                return items[idx].id;
            }
        }
        return "";
    }

    std::string matchQuery(
        const std::string &query,
        const std::vector<TypeaheadItem> &items,
        const std::string &currentId = "")
    {
        if (query.empty()) {
            buffer.clear();
            return "";
        }
        std::string normQ = foldDiacriticsLower(query);
        buffer = normQ;
        if (items.empty()) return "";
        int n = static_cast<int>(items.size());
        int startIdx = 0;
        if (!currentId.empty()) {
            for (int i = 0; i < n; ++i) {
                if (items[i].id == currentId) { startIdx = i; break; }
            }
        }
        for (int pass = 0; pass < 2; ++pass) {
            for (int k = 0; k < n; ++k) {
                int idx = (startIdx + k) % n;
                if (items[idx].disabled) continue;
                std::string itemNorm = foldDiacriticsLower(items[idx].text);
                if (pass == 0 && itemNorm.rfind(normQ, 0) == 0) return items[idx].id;
                if (pass == 1 && itemNorm.find(normQ) != std::string::npos) return items[idx].id;
            }
        }
        return "";
    }
};

// ---------------------------------------------------------------------------
// 7. Virtualizer Primitive (O(log N) Fenwick tree for 100k+ dynamic rows)
// ---------------------------------------------------------------------------
struct VirtualItemRange {
    int index = 0;
    float offset = 0.0f;
    float size = 0.0f;
};

struct VirtualSnapshot {
    float totalSize = 0.0f;
    int startIndex = 0;
    int endIndex = -1;
    int stickyIndex = -1;
    std::vector<VirtualItemRange> visibleItems;
};

class VirtualizerState {
public:
    int count = 0;
    float estimateSize = 44.0f;
    int overscan = 3;
    std::vector<float> itemSizes;
    std::vector<double> bit; // Fenwick tree (1-indexed)
    std::vector<int> stickyIndices;
    float scrollOffset = 0.0f;
    float viewportSize = 400.0f;

    void setCount(int n, float estSize = 44.0f) {
        estimateSize = std::max(1.0f, estSize);
        if (n == count && static_cast<int>(itemSizes.size()) == n) return;
        count = std::max(0, n);
        itemSizes.assign(count, estimateSize);
        rebuildBit();
    }

    void rebuildBit() {
        bit.assign(count + 1, 0.0);
        for (int i = 0; i < count; ++i) {
            addBit(i, itemSizes[i]);
        }
    }

    void addBit(int idx, double delta) {
        int i = idx + 1;
        while (i <= count) {
            bit[i] += delta;
            i += (i & -i);
        }
    }

    void measureItem(int idx, float actualSize) {
        if (idx < 0 || idx >= count) return;
        float clamped = std::max(1.0f, actualSize);
        double delta = static_cast<double>(clamped) - static_cast<double>(itemSizes[idx]);
        if (std::abs(delta) > 0.01) {
            itemSizes[idx] = clamped;
            addBit(idx, delta);
        }
    }

    double offsetOf(int idx) const {
        if (idx <= 0 || count == 0) return 0.0;
        int i = std::min(idx, count);
        double sum = 0.0;
        while (i > 0) {
            sum += bit[i];
            i -= (i & -i);
        }
        return sum;
    }

    double totalSize() const {
        return offsetOf(count);
    }

    int indexAtOffset(double target) const {
        if (count == 0 || target <= 0.0) return 0;
        if (target >= totalSize()) return count - 1;
        int idx = 0;
        double acc = 0.0;
        int bitMask = 1;
        while ((bitMask << 1) <= count) bitMask <<= 1;
        for (int k = bitMask; k > 0; k >>= 1) {
            if (idx + k <= count && acc + bit[idx + k] <= target) {
                acc += bit[idx + k];
                idx += k;
            }
        }
        return std::min(idx, count - 1);
    }

    float scrollToIndex(int idx, const std::string &align = "auto") {
        if (count == 0) return 0.0f;
        int i = std::clamp(idx, 0, count - 1);
        float itemStart = static_cast<float>(offsetOf(i));
        float itemSz = itemSizes[i];
        float itemEnd = itemStart + itemSz;
        float maxScroll = std::max(0.0f, static_cast<float>(totalSize()) - viewportSize);

        if (align == "start") scrollOffset = itemStart;
        else if (align == "center") scrollOffset = itemStart - (viewportSize - itemSz) * 0.5f;
        else if (align == "end") scrollOffset = itemEnd - viewportSize;
        else { // auto
            if (itemStart < scrollOffset) scrollOffset = itemStart;
            else if (itemEnd > scrollOffset + viewportSize) scrollOffset = itemEnd - viewportSize;
        }
        scrollOffset = std::clamp(scrollOffset, 0.0f, maxScroll);
        return scrollOffset;
    }

    VirtualSnapshot computeSnapshot() const {
        VirtualSnapshot snap;
        snap.totalSize = static_cast<float>(totalSize());
        if (count == 0) return snap;

        int rawStart = indexAtOffset(scrollOffset);
        int rawEnd = indexAtOffset(scrollOffset + viewportSize);
        snap.startIndex = std::max(0, rawStart - overscan);
        snap.endIndex = std::min(count - 1, rawEnd + overscan);

        double curOff = offsetOf(snap.startIndex);
        snap.visibleItems.reserve(snap.endIndex - snap.startIndex + 1);
        for (int i = snap.startIndex; i <= snap.endIndex; ++i) {
            float sz = itemSizes[i];
            snap.visibleItems.push_back({i, static_cast<float>(curOff), sz});
            curOff += sz;
        }

        // Sticky header lookup: largest sticky index whose offset <= scrollOffset
        for (int si : stickyIndices) {
            if (si >= 0 && si < count && offsetOf(si) <= scrollOffset + 0.5) {
                snap.stickyIndex = si;
            }
        }
        return snap;
    }
};

// ---------------------------------------------------------------------------
// 8. Reorder Primitive (Pointer DnD + Keyboard Space/Arrows/Escape cancel)
// ---------------------------------------------------------------------------
class ReorderState {
public:
    std::vector<std::string> items;
    std::vector<std::string> snapshotBeforeGrab;
    int grabbedIndex = -1;
    int draggingIndex = -1;
    float dragStartPos = 0.0f;
    float dragDelta = 0.0f;
    bool dragThresholdMet = false;
    std::string announcement = "";

    bool startDrag(int idx, float pointerPos) {
        if (idx < 0 || idx >= static_cast<int>(items.size())) return false;
        snapshotBeforeGrab = items;
        draggingIndex = idx;
        dragStartPos = pointerPos;
        dragDelta = 0.0f;
        dragThresholdMet = false;
        return true;
    }

    bool updateDrag(float pointerPos, const std::vector<float> &itemMidpoints) {
        if (draggingIndex < 0) return false;
        dragDelta = pointerPos - dragStartPos;
        if (!dragThresholdMet && std::abs(dragDelta) < 4.0f) return false;
        dragThresholdMet = true;
        int n = static_cast<int>(items.size());
        int target = draggingIndex;
        for (int i = 0; i < n && i < static_cast<int>(itemMidpoints.size()); ++i) {
            if (i < draggingIndex && pointerPos < itemMidpoints[i]) { target = i; break; }
            if (i > draggingIndex && pointerPos > itemMidpoints[i]) { target = i; }
        }
        if (target != draggingIndex) {
            std::string moved = items[draggingIndex];
            items.erase(items.begin() + draggingIndex);
            items.insert(items.begin() + target, moved);
            draggingIndex = target;
            dragStartPos = pointerPos;
            dragDelta = 0.0f;
            return true;
        }
        return false;
    }

    void endDrag() {
        draggingIndex = -1;
        dragThresholdMet = false;
        dragDelta = 0.0f;
        snapshotBeforeGrab.clear();
    }

    bool handleKey(const std::string &key, int focusedIndex, int &outNewFocusIndex) {
        outNewFocusIndex = focusedIndex;
        int n = static_cast<int>(items.size());
        if (n == 0) return false;

        if (key == " " || key == "Space" || key == "Enter") {
            if (grabbedIndex < 0) {
                if (focusedIndex < 0 || focusedIndex >= n) return false;
                snapshotBeforeGrab = items;
                grabbedIndex = focusedIndex;
                announcement = "Grabbed item at position " + std::to_string(grabbedIndex + 1) + " of " + std::to_string(n);
                return true;
            } else {
                announcement = "Dropped item at position " + std::to_string(grabbedIndex + 1) + " of " + std::to_string(n);
                outNewFocusIndex = grabbedIndex;
                grabbedIndex = -1;
                snapshotBeforeGrab.clear();
                return true;
            }
        }
        if (grabbedIndex < 0) return false;

        if (key == "Escape") {
            if (!snapshotBeforeGrab.empty()) {
                for (int i = 0; i < static_cast<int>(snapshotBeforeGrab.size()); ++i) {
                    if (snapshotBeforeGrab[i] == items[grabbedIndex]) { outNewFocusIndex = i; break; }
                }
                items = snapshotBeforeGrab;
            }
            grabbedIndex = -1;
            snapshotBeforeGrab.clear();
            announcement = "Reorder cancelled";
            return true;
        }

        int target = grabbedIndex;
        if (key == "ArrowUp" || key == "ArrowLeft") target = std::max(0, grabbedIndex - 1);
        else if (key == "ArrowDown" || key == "ArrowRight") target = std::min(n - 1, grabbedIndex + 1);
        else if (key == "Home") target = 0;
        else if (key == "End") target = n - 1;
        else return false;

        if (target != grabbedIndex) {
            std::string moved = items[grabbedIndex];
            items.erase(items.begin() + grabbedIndex);
            items.insert(items.begin() + target, moved);
            grabbedIndex = target;
            outNewFocusIndex = target;
            announcement = "Moved to position " + std::to_string(target + 1) + " of " + std::to_string(n);
            return true;
        }
        return true;
    }

    bool cancel() {
        if (grabbedIndex >= 0 || draggingIndex >= 0) {
            if (!snapshotBeforeGrab.empty()) items = snapshotBeforeGrab;
            grabbedIndex = -1;
            draggingIndex = -1;
            dragThresholdMet = false;
            snapshotBeforeGrab.clear();
            announcement = "Reorder cancelled";
            return true;
        }
        return false;
    }
};

// ---------------------------------------------------------------------------
// 9. Resize Primitive (split-pane bounds, collapsible, pointer & keyboard)
// ---------------------------------------------------------------------------
class ResizeState {
public:
    std::vector<float> sizes = {50.0f, 50.0f}; // percentages or pixels
    std::vector<float> minSizes = {10.0f, 10.0f};
    std::vector<float> maxSizes = {90.0f, 90.0f};
    std::vector<bool> collapsible = {false, false};
    float step = 2.0f;
    float coarseStep = 10.0f;
    bool vertical = false;

    bool resizeHandle(int handleIdx, float delta) {
        if (handleIdx < 0 || handleIdx + 1 >= static_cast<int>(sizes.size())) return false;
        float a = sizes[handleIdx];
        float b = sizes[handleIdx + 1];
        float sum = a + b;

        float minA = (handleIdx < static_cast<int>(minSizes.size())) ? minSizes[handleIdx] : 5.0f;
        float maxA = (handleIdx < static_cast<int>(maxSizes.size())) ? maxSizes[handleIdx] : sum;
        float minB = (handleIdx + 1 < static_cast<int>(minSizes.size())) ? minSizes[handleIdx + 1] : 5.0f;
        float maxB = (handleIdx + 1 < static_cast<int>(maxSizes.size())) ? maxSizes[handleIdx + 1] : sum;

        float nextA = a + delta;
        if (handleIdx < static_cast<int>(collapsible.size()) && collapsible[handleIdx] && nextA < minA * 0.5f) {
            nextA = 0.0f;
        } else {
            nextA = std::clamp(nextA, minA, maxA);
        }

        float nextB = sum - nextA;
        if (nextB < minB) {
            nextB = minB;
            nextA = std::clamp(sum - nextB, (collapsible[handleIdx] && nextA == 0.0f) ? 0.0f : minA, maxA);
            nextB = sum - nextA;
        } else if (nextB > maxB) {
            nextB = maxB;
            nextA = std::clamp(sum - nextB, minA, maxA);
            nextB = sum - nextA;
        }

        if (std::abs(nextA - a) < 1e-4f && std::abs(nextB - b) < 1e-4f) return false;
        sizes[handleIdx] = nextA;
        sizes[handleIdx + 1] = nextB;
        return true;
    }

    bool handleKey(int handleIdx, const std::string &key, bool shift) {
        float s = shift ? coarseStep : step;
        if ((!vertical && key == "ArrowRight") || (vertical && key == "ArrowDown")) return resizeHandle(handleIdx, s);
        if ((!vertical && key == "ArrowLeft") || (vertical && key == "ArrowUp")) return resizeHandle(handleIdx, -s);
        if (key == "Home") return resizeHandle(handleIdx, -1e6f);
        if (key == "End") return resizeHandle(handleIdx, 1e6f);
        if (key == "Enter" && handleIdx < static_cast<int>(collapsible.size()) && collapsible[handleIdx]) {
            if (sizes[handleIdx] <= 0.1f) {
                float restore = (handleIdx < static_cast<int>(minSizes.size())) ? minSizes[handleIdx] : 20.0f;
                return resizeHandle(handleIdx, restore);
            } else {
                return resizeHandle(handleIdx, -sizes[handleIdx]);
            }
        }
        return false;
    }
};

// ---------------------------------------------------------------------------
// 10. Slider Primitive (multi-thumb, step rounding, minStepsBetweenThumbs, RTL)
// ---------------------------------------------------------------------------
class SliderState {
public:
    std::vector<double> values = {50.0};
    double min = 0.0;
    double max = 100.0;
    double step = 1.0;
    int minStepsBetweenThumbs = 0;
    bool vertical = false;
    bool rtl = false;
    bool disabled = false;

    double snapValue(double v) const {
        double clamped = std::clamp(v, min, max);
        if (step <= 0.0) return clamped;
        double steps = std::round((clamped - min) / step);
        double snapped = min + steps * step;
        // Eliminate IEEE-754 drift (e.g. 0.30000000000000004)
        double prec = std::pow(10.0, 6);
        snapped = std::round(snapped * prec) / prec;
        return std::clamp(snapped, min, max);
    }

    bool setThumbValue(int thumbIdx, double rawVal) {
        if (disabled || thumbIdx < 0 || thumbIdx >= static_cast<int>(values.size())) return false;
        double v = snapValue(rawVal);
        double minGap = static_cast<double>(minStepsBetweenThumbs) * step;
        if (thumbIdx > 0) {
            v = std::max(v, values[thumbIdx - 1] + minGap);
        }
        if (thumbIdx + 1 < static_cast<int>(values.size())) {
            v = std::min(v, values[thumbIdx + 1] - minGap);
        }
        v = snapValue(v);
        if (std::abs(v - values[thumbIdx]) < 1e-9) return false;
        values[thumbIdx] = v;
        return true;
    }

    bool setFromRatio(int thumbIdx, double ratio) {
        double r = std::clamp(ratio, 0.0, 1.0);
        if (!vertical && rtl) r = 1.0 - r;
        if (vertical) r = 1.0 - r;
        return setThumbValue(thumbIdx, min + r * (max - min));
    }

    int nearestThumbForRatio(double ratio) const {
        double r = std::clamp(ratio, 0.0, 1.0);
        if (!vertical && rtl) r = 1.0 - r;
        if (vertical) r = 1.0 - r;
        double target = min + r * (max - min);
        int best = 0;
        double bestDist = 1e18;
        for (int i = 0; i < static_cast<int>(values.size()); ++i) {
            double d = std::abs(values[i] - target);
            if (d < bestDist) { bestDist = d; best = i; }
        }
        return best;
    }

    bool handleKey(int thumbIdx, const std::string &key, bool shift) {
        if (disabled || thumbIdx < 0 || thumbIdx >= static_cast<int>(values.size())) return false;
        double dir = (!vertical && rtl) ? -1.0 : 1.0;
        double mult = shift ? 10.0 : 1.0;
        if (key == "ArrowRight") return setThumbValue(thumbIdx, values[thumbIdx] + step * mult * dir);
        if (key == "ArrowLeft")  return setThumbValue(thumbIdx, values[thumbIdx] - step * mult * dir);
        if (key == "ArrowUp")    return setThumbValue(thumbIdx, values[thumbIdx] + step * mult);
        if (key == "ArrowDown")  return setThumbValue(thumbIdx, values[thumbIdx] - step * mult);
        if (key == "PageUp")     return setThumbValue(thumbIdx, values[thumbIdx] + std::max(step, (max - min) * 0.1));
        if (key == "PageDown")   return setThumbValue(thumbIdx, values[thumbIdx] - std::max(step, (max - min) * 0.1));
        if (key == "Home")       return setThumbValue(thumbIdx, min);
        if (key == "End")        return setThumbValue(thumbIdx, max);
        return false;
    }
};

// ---------------------------------------------------------------------------
// 11. Tabs Primitive (RovingFocus + Selection + Automatic/Manual Activation)
// ---------------------------------------------------------------------------
class TabsState {
public:
    RovingFocusState roving;
    SelectionState selection;
    std::string activationMode = "automatic"; // "automatic" | "manual"

    TabsState() {
        roving.orientation = "horizontal";
        selection.mode = "single";
        selection.disallowEmpty = true;
    }

    void syncItems(const std::vector<RovingItem> &items) {
        roving.items = items;
        selection.disabledIds.clear();
        for (const auto &it : items) {
            if (it.disabled) selection.disabledIds.insert(it.id);
        }
        roving.ensureValidActive();
        if (selection.selectedIds.empty() && !roving.activeId.empty()) {
            selection.select(roving.activeId);
        }
    }

    std::string activeTab() const {
        return selection.selectedIds.empty() ? roving.activeId : selection.selectedIds[0];
    }

    bool handleKey(const std::string &key, std::string &outFocusedId) {
        if (roving.handleKey(key, outFocusedId)) {
            if (activationMode == "automatic") {
                selection.select(outFocusedId);
            }
            return true;
        }
        if (key == "Enter" || key == " " || key == "Space") {
            if (!roving.activeId.empty()) {
                selection.select(roving.activeId);
                return true;
            }
        }
        return false;
    }

    bool clickTab(const std::string &id) {
        for (const auto &it : roving.items) {
            if (it.id == id && it.disabled) return false;
        }
        roving.activeId = id;
        return selection.select(id);
    }
};

// ---------------------------------------------------------------------------
// 12. Toast Manager (queue, maxVisible, hover/focus pause, auto-dismiss, swipe)
// ---------------------------------------------------------------------------
struct ToastItem {
    std::string id;
    std::string title;
    std::string description;
    std::string variant = "default";
    double durationMs = 4000.0;
    double remainingMs = 4000.0;
    bool paused = false;
};

class ToastManager {
public:
    std::vector<ToastItem> toasts;
    int maxVisible = 4;
    int nextId = 1;

    std::string add(const std::string &title, const std::string &desc = "", const std::string &variant = "default", double durationMs = 4000.0) {
        std::string id = "toast_" + std::to_string(nextId++);
        toasts.push_back({id, title, desc, variant, durationMs, durationMs, false});
        while (static_cast<int>(toasts.size()) > maxVisible) {
            toasts.erase(toasts.begin());
        }
        return id;
    }

    void dismiss(const std::string &id) {
        toasts.erase(std::remove_if(toasts.begin(), toasts.end(), [&](const ToastItem &t) {
            return t.id == id;
        }), toasts.end());
    }

    void dismissAll() { toasts.clear(); }

    void setPaused(const std::string &id, bool p) {
        for (auto &t : toasts) {
            if (t.id == id) t.paused = p;
        }
    }

    void pauseAll(bool p) {
        for (auto &t : toasts) t.paused = p;
    }

    bool tick(double deltaMs) {
        bool changed = false;
        for (auto it = toasts.begin(); it != toasts.end();) {
            if (!it->paused && it->durationMs > 0.0) {
                it->remainingMs -= deltaMs;
                if (it->remainingMs <= 0.0) {
                    it = toasts.erase(it);
                    changed = true;
                    continue;
                }
            }
            ++it;
        }
        return changed;
    }
};

// ---------------------------------------------------------------------------
// 13. Tooltip Delay Group (hover delay, skip-delay warmup, interactive hover)
// ---------------------------------------------------------------------------
class TooltipDelayGroup {
public:
    double openDelayMs = 400.0;
    double skipDelayMs = 300.0;
    double skipWindowRemainingMs = 0.0;
    std::string pendingTriggerId = "";
    double pendingElapsedMs = 0.0;
    std::string openTriggerId = "";
    bool pointerInsideTooltip = false;

    void onTriggerHoverStart(const std::string &triggerId) {
        if (openTriggerId == triggerId) return;
        if (skipWindowRemainingMs > 0.0 || !openTriggerId.empty()) {
            // Instant open during skip-delay window!
            openTriggerId = triggerId;
            pendingTriggerId.clear();
            pendingElapsedMs = 0.0;
        } else {
            pendingTriggerId = triggerId;
            pendingElapsedMs = 0.0;
        }
    }

    void onTriggerFocus(const std::string &triggerId) {
        openTriggerId = triggerId;
        pendingTriggerId.clear();
        pendingElapsedMs = 0.0;
    }

    void onTriggerHoverEnd(const std::string &triggerId) {
        if (pendingTriggerId == triggerId) {
            pendingTriggerId.clear();
            pendingElapsedMs = 0.0;
        }
        if (openTriggerId == triggerId && !pointerInsideTooltip) {
            openTriggerId.clear();
            skipWindowRemainingMs = skipDelayMs;
        }
    }

    void onTooltipHoverChange(bool inside) {
        pointerInsideTooltip = inside;
        if (!inside && !openTriggerId.empty()) {
            // stays open only if trigger is hovered; caller manages explicit leave
        }
    }

    bool handleEscape() {
        if (!openTriggerId.empty() || !pendingTriggerId.empty()) {
            openTriggerId.clear();
            pendingTriggerId.clear();
            skipWindowRemainingMs = 0.0;
            return true;
        }
        return false;
    }

    bool tick(double deltaMs) {
        bool changed = false;
        if (skipWindowRemainingMs > 0.0) {
            skipWindowRemainingMs = std::max(0.0, skipWindowRemainingMs - deltaMs);
        }
        if (!pendingTriggerId.empty()) {
            pendingElapsedMs += deltaMs;
            if (pendingElapsedMs >= openDelayMs) {
                openTriggerId = pendingTriggerId;
                pendingTriggerId.clear();
                pendingElapsedMs = 0.0;
                changed = true;
            }
        }
        return changed;
    }
};

// ---------------------------------------------------------------------------
// 14. Agent Accessibility Audit & Snapshot Result
// ---------------------------------------------------------------------------
struct AgentAuditResult {
    bool ok = true;
    std::vector<std::string> violations;
};

inline AgentAuditResult auditHitRegionsAccessibility(
    const std::vector<UiHitRegion> &regions,
    const PopupStack &popups,
    const FocusScopeStack &focusScopes,
    const std::string &focusedKey,
    float viewportW,
    float viewportH)
{
    AgentAuditResult res;
    std::unordered_set<std::string> seenTestIds;

    for (const auto &hr : regions) {
        bool isInteractive = hr.hasClick || hr.hasChange || hr.role == "slider" || hr.role == "checkbox" ||
                             hr.role == "switch" || hr.role == "tab" || hr.role == "combobox" || hr.role == "radio";
        if (isInteractive && !hr.disabled) {
            if (hr.width <= 0.0f || hr.height <= 0.0f) {
                res.violations.push_back("Interactive element '" + hr.tag + "' (id=" + std::to_string(hr.id) + ") has zero bounding box");
            }
            std::string label = hr.accessibleName.empty() ? hr.inputValue : hr.accessibleName;
            if (label.empty() && hr.placeholder.empty()) {
                res.violations.push_back("Interactive element '" + hr.tag + "' (id=" + std::to_string(hr.id) + ") missing accessible label");
            }
        }
        if (!hr.testId.empty()) {
            if (!seenTestIds.insert(hr.testId).second) {
                res.violations.push_back("Duplicate data-testid='" + hr.testId + "'");
            }
        }
        if (hr.role == "slider") {
            if (hr.numValue < hr.numMin || hr.numValue > hr.numMax) {
                res.violations.push_back("Slider value " + std::to_string(hr.numValue) + " out of bounds [" + std::to_string(hr.numMin) + ", " + std::to_string(hr.numMax) + "]");
            }
        }
    }

    for (const auto &p : popups.stack) {
        if (p.bounds.width > 0 && p.bounds.height > 0) {
            if (p.bounds.x < -1.0f || p.bounds.y < -1.0f ||
                p.bounds.x + p.bounds.width > viewportW + 1.0f ||
                p.bounds.y + p.bounds.height > viewportH + 1.0f) {
                res.violations.push_back("Open popup '" + p.id + "' overflows viewport bounds");
            }
        }
    }

    if (const auto *trap = focusScopes.activeTrap()) {
        if (!focusedKey.empty()) {
            bool inside = (focusedKey == trap->containerId);
            for (const auto &fid : trap->focusableIds) {
                if (fid == focusedKey) { inside = true; break; }
            }
            if (!inside) {
                res.violations.push_back("Focus escaped active modal trap '" + trap->id + "' (focused='" + focusedKey + "')");
            }
        }
    }

    res.ok = res.violations.empty();
    return res;
}

} // namespace MemeUI
