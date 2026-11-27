#pragma once

#include "MemePlatform.h"

#include <cctype>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace MemePermissions {

inline std::string normalizeCapability(const std::string &raw) {
    std::string clean;
    clean.reserve(raw.size());
    for (unsigned char c : raw) {
        if (std::isalnum(c)) clean.push_back(static_cast<char>(std::tolower(c)));
    }
    if (clean == "notifications" || clean == "notification" || clean == "notify" || clean == "push" || clean == "postnotifications") {
        return "notifications";
    }
    if (clean == "camera" || clean == "cam" || clean == "video") return "camera";
    if (clean == "microphone" || clean == "mic" || clean == "audio" || clean == "recordaudio") return "microphone";
    if (clean == "photos" || clean == "photo" || clean == "photolibrary" || clean == "gallery" || clean == "media" || clean == "readmediaimages") {
        return "photos";
    }
    if (clean == "locationalways" || clean == "backgroundlocation" || clean == "accessbackgroundlocation") return "locationAlways";
    if (clean == "location" || clean == "loc" || clean == "geolocation" || clean == "locationwheninuse" ||
        clean == "accessfinelocation" || clean == "accesscoarselocation") {
        return "location";
    }
    if (clean == "biometrics" || clean == "biometric" || clean == "faceid" || clean == "touchid" || clean == "usebiometric") {
        return "biometrics";
    }
    if (clean == "contacts" || clean == "contact" || clean == "addressbook" || clean == "readcontacts") return "contacts";
    if (clean == "calendar" || clean == "calendars" || clean == "events" || clean == "readcalendar") return "calendar";
    if (clean == "bluetooth" || clean == "ble" || clean == "bluetoothscan" || clean == "bluetoothconnect") return "bluetooth";
    if (clean == "sensors" || clean == "sensor" || clean == "motion" || clean == "accelerometer" || clean == "gyroscope" || clean == "activityrecognition") {
        return "sensors";
    }
    if (clean == "clipboard" || clean == "clip" || clean == "clipboardread" || clean == "clipboardwrite") return "clipboard";
    if (clean == "storage" || clean == "files" || clean == "file" || clean == "externalstorage" || clean == "readexternalstorage") {
        return "storage";
    }
    return MemePlatform::toLowerTrimmed(raw);
}

inline const std::vector<std::string> &allStandardCapabilities() {
    static const std::vector<std::string> kCaps = {
        "notifications", "camera", "microphone", "photos", "location", "locationAlways",
        "biometrics", "contacts", "calendar", "bluetooth", "sensors", "clipboard", "storage"
    };
    return kCaps;
}

inline bool isKnownCapability(const std::string &norm) {
    for (const auto &c : allStandardCapabilities()) {
        if (c == norm) return true;
    }
    return false;
}

inline std::string normalizeStateString(const std::string &raw) {
    std::string s = MemePlatform::toLowerTrimmed(raw);
    if (s == "granted" || s == "allow" || s == "allowed" || s == "authorized" || s == "yes" || s == "true") return "granted";
    if (s == "limited" || s == "provisional") return "limited";
    if (s == "prompt-with-rationale" || s == "promptwithrationale" || s == "rationale") return "prompt-with-rationale";
    if (s == "prompt" || s == "notdetermined" || s == "default" || s == "ask") return "prompt";
    if (s == "restricted") return "restricted";
    if (s == "unavailable" || s == "unsupported") return "unavailable";
    if (s == "denied" || s == "deny" || s == "blocked" || s == "no" || s == "false") return "denied";
    return "prompt";
}

struct PermissionStatus {
    std::string permission;
    std::string state = "prompt"; // granted | denied | prompt | prompt-with-rationale | restricted | limited | unavailable
    bool granted = false;
    bool canAskAgain = true;
    bool restricted = false;
    bool limited = false;
    std::string platform;
    std::string reason;
    std::string error;

    void finalizeFlags() {
        state = normalizeStateString(state);
        if (platform.empty()) platform = MemePlatform::currentPlatform();
        granted = (state == "granted" || state == "limited");
        limited = (state == "limited");
        restricted = (state == "restricted");
        if (state == "denied" || state == "restricted" || state == "unavailable" || state == "granted") {
            if (state != "granted" && state != "prompt" && state != "prompt-with-rationale") {
                canAskAgain = false;
            }
        }
        if (state == "granted") canAskAgain = false;
        if (state == "prompt" || state == "prompt-with-rationale") canAskAgain = true;
    }
};

struct AndroidPermissionEntry {
    std::string name;
    int maxSdkVersion = 0; // 0 => omitted
    std::string flags;
};

inline std::vector<AndroidPermissionEntry> androidPermissionsFor(const std::string &cap, const std::string &mode = "") {
    std::string c = normalizeCapability(cap);
    if (c == "notifications") return {{"android.permission.POST_NOTIFICATIONS", 0, ""}};
    if (c == "camera") return {{"android.permission.CAMERA", 0, ""}};
    if (c == "microphone") return {{"android.permission.RECORD_AUDIO", 0, ""}};
    if (c == "photos") {
        return {
            {"android.permission.READ_MEDIA_IMAGES", 0, ""},
            {"android.permission.READ_EXTERNAL_STORAGE", 32, ""}
        };
    }
    if (c == "location") {
        if (MemePlatform::toLowerTrimmed(mode) == "coarse") {
            return {{"android.permission.ACCESS_COARSE_LOCATION", 0, ""}};
        }
        return {
            {"android.permission.ACCESS_FINE_LOCATION", 0, ""},
            {"android.permission.ACCESS_COARSE_LOCATION", 0, ""}
        };
    }
    if (c == "locationAlways") {
        return {
            {"android.permission.ACCESS_FINE_LOCATION", 0, ""},
            {"android.permission.ACCESS_COARSE_LOCATION", 0, ""},
            {"android.permission.ACCESS_BACKGROUND_LOCATION", 0, ""}
        };
    }
    if (c == "biometrics") return {{"android.permission.USE_BIOMETRIC", 0, ""}};
    if (c == "contacts") return {{"android.permission.READ_CONTACTS", 0, ""}};
    if (c == "calendar") return {{"android.permission.READ_CALENDAR", 0, ""}};
    if (c == "bluetooth") {
        return {
            {"android.permission.BLUETOOTH_SCAN", 0, "neverForLocation"},
            {"android.permission.BLUETOOTH_CONNECT", 0, ""},
            {"android.permission.BLUETOOTH", 30, ""}
        };
    }
    if (c == "sensors") return {{"android.permission.ACTIVITY_RECOGNITION", 0, ""}};
    if (c == "storage") {
        return {
            {"android.permission.READ_MEDIA_IMAGES", 0, ""},
            {"android.permission.READ_EXTERNAL_STORAGE", 32, ""}
        };
    }
    if (c == "vibrator" || c == "haptics") return {{"android.permission.VIBRATE", 0, ""}};
    if (c == "network") {
        return {
            {"android.permission.INTERNET", 0, ""},
            {"android.permission.ACCESS_NETWORK_STATE", 0, ""}
        };
    }
    return {};
}

inline std::pair<std::string, std::string> appleUsagePlistKeyFor(const std::string &cap, const std::string &appDisplayName) {
    std::string c = normalizeCapability(cap);
    std::string app = appDisplayName.empty() ? "MEME App" : appDisplayName;
    if (c == "camera") return {"NSCameraUsageDescription", app + " requires camera access when capturing media."};
    if (c == "microphone") return {"NSMicrophoneUsageDescription", app + " requires microphone access when recording audio."};
    if (c == "photos") return {"NSPhotoLibraryUsageDescription", app + " accesses photos when you choose images."};
    if (c == "location") return {"NSLocationWhenInUseUsageDescription", app + " requests location access when invoked."};
    if (c == "locationAlways") return {"NSLocationAlwaysAndWhenInUseUsageDescription", app + " uses your location for background updates."};
    if (c == "biometrics") return {"NSFaceIDUsageDescription", app + " uses Face ID / biometrics for authentication."};
    if (c == "contacts") return {"NSContactsUsageDescription", app + " reads contacts when requested by you."};
    if (c == "calendar") return {"NSCalendarsUsageDescription", app + " accesses your calendar events when requested."};
    if (c == "bluetooth") return {"NSBluetoothAlwaysUsageDescription", app + " connects to nearby Bluetooth devices."};
    if (c == "sensors") return {"NSMotionUsageDescription", app + " uses motion sensors when requested."};
    return {"", ""};
}

inline std::string macEntitlementKeyFor(const std::string &cap) {
    std::string c = normalizeCapability(cap);
    if (c == "camera") return "com.apple.security.device.camera";
    if (c == "microphone") return "com.apple.security.device.audio-input";
    if (c == "location" || c == "locationAlways") return "com.apple.security.personal-information.location";
    if (c == "photos") return "com.apple.security.personal-information.photos-library";
    if (c == "contacts") return "com.apple.security.personal-information.addressbook";
    if (c == "calendar") return "com.apple.security.personal-information.calendars";
    if (c == "bluetooth") return "com.apple.security.device.bluetooth";
    if (c == "network") return "com.apple.security.network.client";
    return "";
}

inline std::vector<std::string> webPolicyDirectivesFor(const std::string &cap) {
    std::string c = normalizeCapability(cap);
    if (c == "camera") return {"camera=(self)"};
    if (c == "microphone") return {"microphone=(self)"};
    if (c == "location" || c == "locationAlways") return {"geolocation=(self)"};
    if (c == "clipboard") return {"clipboard-read=(self)", "clipboard-write=(self)"};
    if (c == "sensors") return {"accelerometer=(self)", "gyroscope=(self)"};
    return {};
}

inline std::string windowsConsentStoreKeyFor(const std::string &cap) {
    std::string c = normalizeCapability(cap);
    if (c == "camera") return "webcam";
    if (c == "microphone") return "microphone";
    if (c == "location" || c == "locationAlways") return "location";
    if (c == "contacts") return "contacts";
    if (c == "calendar") return "appointments";
    return "";
}

inline std::string windowsSettingsUriFor(const std::string &cap) {
    std::string c = normalizeCapability(cap);
    if (c == "camera") return "ms-settings:privacy-webcam";
    if (c == "microphone") return "ms-settings:privacy-microphone";
    if (c == "location" || c == "locationAlways") return "ms-settings:privacy-location";
    if (c == "notifications") return "ms-settings:notifications";
    if (c == "contacts") return "ms-settings:privacy-contacts";
    if (c == "calendar") return "ms-settings:privacy-calendar";
    return "ms-settings:privacy";
}

inline std::string macSettingsUriFor(const std::string &cap) {
    std::string c = normalizeCapability(cap);
    if (c == "camera") return "x-apple.systempreferences:com.apple.preference.security?Privacy_Camera";
    if (c == "microphone") return "x-apple.systempreferences:com.apple.preference.security?Privacy_Microphone";
    if (c == "photos") return "x-apple.systempreferences:com.apple.preference.security?Privacy_Photos";
    if (c == "location" || c == "locationAlways") return "x-apple.systempreferences:com.apple.preference.security?Privacy_LocationServices";
    if (c == "contacts") return "x-apple.systempreferences:com.apple.preference.security?Privacy_Contacts";
    if (c == "calendar") return "x-apple.systempreferences:com.apple.preference.security?Privacy_Calendars";
    if (c == "notifications") return "x-apple.systempreferences:com.apple.Notifications-Settings.extension";
    return "x-apple.systempreferences:com.apple.preference.security?Privacy";
}

inline bool isSupportedOnPlatform(const std::string &cap, const std::string &plat = "") {
    std::string c = normalizeCapability(cap);
    std::string p = plat.empty() ? MemePlatform::currentPlatform() : MemePlatform::normalizePlatformName(plat);
    if (c == "notifications" || c == "camera" || c == "microphone" || c == "photos" ||
        c == "location" || c == "sensors" || c == "clipboard" || c == "storage") {
        return true;
    }
    if (c == "biometrics") return (p == "ios" || p == "android" || p == "mac" || p == "windows");
    if (c == "locationAlways" || c == "contacts" || c == "calendar" || c == "bluetooth") {
        return (p == "ios" || p == "android" || p == "mac" || p == "windows" || p == "linux");
    }
    return isKnownCapability(c);
}

struct PlatformPermissionsConfig {
    bool hasPermissionsBlock = false;
    bool strictMode = false;
    std::vector<std::string> common;
    std::unordered_map<std::string, std::vector<std::string>> perPlatform; // normalized plat -> caps
    std::unordered_map<std::string, std::map<std::string, std::string>> descriptions; // "common"|plat -> cap -> msg
    std::unordered_map<std::string, std::string> modes; // cap -> mode (e.g. "coarse" vs "fine")

    std::vector<std::string> resolvedForTarget(const std::string &rawTarget) const {
        std::string p = MemePlatform::normalizePlatformName(rawTarget);
        std::vector<std::string> out;
        auto pushUnique = [&](const std::string &raw) {
            std::string n = normalizeCapability(raw);
            if (n.empty()) return;
            for (const auto &ex : out) {
                if (ex == n) return;
            }
            out.push_back(n);
        };
        if (!hasPermissionsBlock) {
            // Backwards-compatible default set when tapri.json has no "permissions" key
            for (const auto &d : {"notifications", "location", "camera", "photos", "microphone", "biometrics", "storage", "clipboard"}) {
                pushUnique(d);
            }
            return out;
        }
        for (const auto &c : common) pushUnique(c);
        auto it = perPlatform.find(p);
        if (it != perPlatform.end()) {
            for (const auto &c : it->second) pushUnique(c);
        }
        return out;
    }

    std::string reasonFor(const std::string &rawTarget, const std::string &cap, const std::string &appTitle) const {
        std::string p = MemePlatform::normalizePlatformName(rawTarget);
        std::string c = normalizeCapability(cap);
        auto itp = descriptions.find(p);
        if (itp != descriptions.end()) {
            auto itc = itp->second.find(c);
            if (itc != itp->second.end() && !itc->second.empty()) return itc->second;
        }
        auto itg = descriptions.find("common");
        if (itg != descriptions.end()) {
            auto itc = itg->second.find(c);
            if (itc != itg->second.end() && !itc->second.empty()) return itc->second;
        }
        return appleUsagePlistKeyFor(c, appTitle).second;
    }
};

struct PermissionRuntimeStore {
    std::mutex mu;
    bool hasDeclaredCapabilities = false;
    bool strictMode = false;
    std::vector<std::string> declaredCapabilities;
    std::unordered_map<std::string, std::string> descriptions;
    std::unordered_map<std::string, std::string> overrides; // headless/test state overrides
};

inline PermissionRuntimeStore &permStore() {
    static PermissionRuntimeStore s;
    return s;
}

inline void setDeclaredPermissions(const std::vector<std::string> &caps, bool strict,
                                   const std::unordered_map<std::string, std::string> &descs = {}) {
    auto &st = permStore();
    std::lock_guard<std::mutex> lock(st.mu);
    st.hasDeclaredCapabilities = true;
    st.strictMode = strict;
    st.declaredCapabilities.clear();
    for (const auto &c : caps) {
        std::string n = normalizeCapability(c);
        if (!n.empty()) st.declaredCapabilities.push_back(n);
    }
    st.descriptions = descs;
}

inline std::vector<std::string> getDeclaredPermissions() {
    auto &st = permStore();
    std::lock_guard<std::mutex> lock(st.mu);
    return st.declaredCapabilities;
}

inline bool isPermissionDeclared(const std::string &cap) {
    auto &st = permStore();
    std::lock_guard<std::mutex> lock(st.mu);
    if (!st.hasDeclaredCapabilities || !st.strictMode) return true;
    std::string n = normalizeCapability(cap);
    for (const auto &c : st.declaredCapabilities) {
        if (c == n) return true;
    }
    return false;
}

inline std::string getPermissionDescription(const std::string &cap, const std::string &appTitle) {
    auto &st = permStore();
    std::string n = normalizeCapability(cap);
    {
        std::lock_guard<std::mutex> lock(st.mu);
        auto it = st.descriptions.find(n);
        if (it != st.descriptions.end() && !it->second.empty()) return it->second;
    }
    return appleUsagePlistKeyFor(n, appTitle).second;
}

inline void setHeadlessPermissionOverride(const std::string &cap, const std::string &stateStr) {
    auto &st = permStore();
    std::lock_guard<std::mutex> lock(st.mu);
    std::string n = normalizeCapability(cap);
    if (stateStr.empty()) st.overrides.erase(n);
    else st.overrides[n] = normalizeStateString(stateStr);
}

inline void clearHeadlessPermissionOverrides() {
    auto &st = permStore();
    std::lock_guard<std::mutex> lock(st.mu);
    st.overrides.clear();
}

} // namespace MemePermissions
