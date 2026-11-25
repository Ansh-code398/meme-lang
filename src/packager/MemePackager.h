#pragma once

#include "ArchiveBuilders.h"
#include "../compiler/NativeCompiler.h"
#include "../hosts/web/MemeWebCanvasHost.h"
#include "../lexer/Lexer.h"
#include "../parser/Parser.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace MemePackager {

struct PackageResult {
    bool success = false;
    std::string target;
    std::string outputPath;
    std::vector<std::string> artifacts;
    std::string error;
    double elapsedMs = 0.0;
};

namespace detail {

inline std::filesystem::path findRepoRoot() {
    std::filesystem::path p = std::filesystem::current_path();
    for (int i = 0; i < 6; ++i) {
        if (std::filesystem::exists(p / "src" / "compiler" / "MemeRuntime.h")) {
            return p;
        }
        if (!p.has_parent_path() || p == p.parent_path()) break;
        p = p.parent_path();
    }
    return std::filesystem::current_path();
}

inline std::string sanitizePkgName(const std::string &raw) {
    std::string out;
    for (unsigned char c : raw) {
        if (std::isalnum(c)) out.push_back(static_cast<char>(std::tolower(c)));
        else if (c == '-' || c == '_') out.push_back('-');
    }
    while (!out.empty() && !std::isalnum(static_cast<unsigned char>(out.front()))) out.erase(out.begin());
    while (!out.empty() && !std::isalnum(static_cast<unsigned char>(out.back()))) out.pop_back();
    if (out.empty()) out = "meme-app";
    return out;
}

inline std::string sanitizeJavaIdent(const std::string &raw) {
    std::string out;
    for (unsigned char c : raw) {
        if (std::isalnum(c)) out.push_back(static_cast<char>(std::tolower(c)));
        else out.push_back('_');
    }
    if (out.empty() || !std::isalpha(static_cast<unsigned char>(out.front()))) {
        out = "app_" + out;
    }
    return out;
}

inline std::string toAppName(const std::string &raw) {
    std::string out;
    bool capNext = true;
    for (unsigned char c : raw) {
        if (std::isalnum(c)) {
            out.push_back(capNext ? static_cast<char>(std::toupper(c)) : static_cast<char>(c));
            capNext = false;
        } else {
            capNext = true;
        }
    }
    if (out.empty()) out = "MemeApp";
    return out;
}

inline int execCmd(const std::string &cmd, std::string &output) {
    output.clear();
    std::string full = cmd + " 2>&1";
    FILE *pipe = popen(full.c_str(), "r");
    if (!pipe) {
        output = "Failed to spawn command: " + cmd;
        return -1;
    }
    char buf[512];
    while (fgets(buf, sizeof(buf), pipe) != nullptr) {
        output += buf;
    }
    int status = pclose(pipe);
    return status;
}

inline std::string findInPathOrPaths(const std::string &bin, const std::vector<std::string> &extra) {
    for (const auto &p : extra) {
        if (std::filesystem::exists(p)) return p;
    }
    std::string out;
    if (execCmd("command -v " + bin, out) == 0) {
        while (!out.empty() && (out.back() == '\n' || out.back() == '\r' || out.back() == ' ')) out.pop_back();
        if (!out.empty()) return out;
    }
    return "";
}

inline std::vector<uint8_t> readFileBytes(const std::filesystem::path &p) {
    std::ifstream ifs(p, std::ios::binary);
    if (!ifs.is_open()) return {};
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
}

inline bool writeFileBytes(const std::filesystem::path &p, const std::vector<uint8_t> &bytes) {
    if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());
    std::ofstream ofs(p, std::ios::binary);
    if (!ofs.is_open()) return false;
    if (!bytes.empty()) ofs.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return ofs.good();
}

inline bool writeFileText(const std::filesystem::path &p, const std::string &txt) {
    if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());
    std::ofstream ofs(p);
    if (!ofs.is_open()) return false;
    ofs << txt;
    return ofs.good();
}

inline std::vector<uint8_t> strToBytes(const std::string &s) {
    return std::vector<uint8_t>(s.begin(), s.end());
}

inline std::filesystem::path findAndroidSdk() {
    std::vector<std::filesystem::path> cands;
    if (const char *h = std::getenv("ANDROID_HOME")) cands.emplace_back(h);
    if (const char *r = std::getenv("ANDROID_SDK_ROOT")) cands.emplace_back(r);
    if (const char *home = std::getenv("HOME")) {
        cands.push_back(std::filesystem::path(home) / "Library" / "Android" / "sdk");
        cands.push_back(std::filesystem::path(home) / "Android" / "Sdk");
    }
    for (const auto &c : cands) {
        if (std::filesystem::exists(c)) return c;
    }
    return {};
}

inline std::filesystem::path newestSubdir(const std::filesystem::path &dir) {
    std::vector<std::filesystem::path> items;
    if (!std::filesystem::exists(dir)) return {};
    for (const auto &e : std::filesystem::directory_iterator(dir)) {
        if (e.is_directory()) items.push_back(e.path());
    }
    if (items.empty()) return {};
    std::sort(items.begin(), items.end());
    return items.back();
}

inline bool buildLinuxElf(const std::filesystem::path &repoRoot, const std::filesystem::path &cppPath,
                          const std::filesystem::path &elfOut, std::string &errOut, bool verbose) {
    std::string zig = findInPathOrPaths("zig", {"/opt/homebrew/bin/zig", "/usr/local/bin/zig"});
    if (zig.empty()) {
        errOut = "zig compiler not found for Linux cross-compilation";
        return false;
    }
    if (std::filesystem::exists(elfOut)) return true;
    std::string cmd = "\"" + zig + "\" c++ -std=c++20 -O3 -DNDEBUG -DMEME_NO_DEPTH_GUARD "
                      "-target x86_64-linux-musl -I\"" + repoRoot.string() + "\" "
                      "\"" + cppPath.string() + "\" -ldl -lpthread -o \"" + elfOut.string() + "\"";
    if (verbose) std::cout << "[package] " << cmd << "\n";
    std::string out;
    if (execCmd(cmd, out) != 0) {
        errOut = "Linux x86_64-musl compilation failed:\n" + out;
        return false;
    }
    return std::filesystem::exists(elfOut);
}

} // namespace detail

inline PackageResult packageForTarget(
    const std::string &sourceFile,
    const std::string &rawTarget,
    const std::string &outDirOrPath = "dist",
    bool verbose = false)
{
    auto t0 = std::chrono::high_resolution_clock::now();
    std::string target = rawTarget;
    for (auto &c : target) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    PackageResult res;
    res.target = target;

    std::filesystem::path srcPath(sourceFile);
    if (!std::filesystem::exists(srcPath)) {
        res.error = "Source file not found: " + sourceFile;
        return res;
    }

    std::string stem = srcPath.stem().string();
    std::string pkgName = detail::sanitizePkgName(stem);
    std::string appName = detail::toAppName(stem);
    if (stem == "App" && srcPath.has_parent_path()) {
        auto parentName = srcPath.parent_path().filename().string();
        if (parentName == "src" && srcPath.parent_path().has_parent_path()) {
            parentName = srcPath.parent_path().parent_path().filename().string();
        }
        if (!parentName.empty() && parentName != "." && parentName != "desktop") {
            pkgName = detail::sanitizePkgName(parentName);
            appName = detail::toAppName(parentName);
        }
    }

    std::filesystem::path repoRoot = detail::findRepoRoot();
    std::filesystem::path outBase = outDirOrPath.empty() ? std::filesystem::path("dist") : std::filesystem::path(outDirOrPath);
    std::filesystem::create_directories(outBase);

    // Parse & generate C++ source once
    std::ifstream ifs(srcPath);
    std::stringstream ss;
    ss << ifs.rdbuf();
    std::string cppCode;
    try {
        Lexer lexer(ss.str());
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto program = parser.parseProgram();
        cppCode = NativeCompiler::generateCpp(program, sourceFile);
    } catch (const std::exception &e) {
        res.error = std::string("Failed to compile MEMElang AST: ") + e.what();
        return res;
    }

    bool usesUi = (cppCode.find("#include \"src/compiler/MemeRuntimeUi.h\"") != std::string::npos);
    std::filesystem::path workDir = std::filesystem::temp_directory_path() / ("meme_pkg_" + pkgName);
    std::filesystem::create_directories(workDir);
    std::filesystem::path cppPath = workDir / (pkgName + "_gen.cpp");
    detail::writeFileText(cppPath, cppCode);

    auto finishOk = [&](const std::string &primary, const std::vector<std::string> &arts) {
        auto t1 = std::chrono::high_resolution_clock::now();
        res.success = true;
        res.outputPath = primary;
        res.artifacts = arts;
        res.elapsedMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        return res;
    };

    if (target == "mac" || target == "macos") {
        std::filesystem::path appBundle = outBase / (appName + ".app");
        std::filesystem::path macOsDir = appBundle / "Contents" / "MacOS";
        std::filesystem::path resDir = appBundle / "Contents" / "Resources";
        std::filesystem::create_directories(macOsDir);
        std::filesystem::create_directories(resDir);
        std::filesystem::path binPath = macOsDir / appName;

        std::string cmd = "clang++ -std=c++20 -O3 -DNDEBUG -DMEME_NO_DEPTH_GUARD -I\"" + repoRoot.string() + "\" ";
        if (usesUi) cmd += "-x objective-c++ ";
        cmd += "\"" + cppPath.string() + "\" ";
        if (usesUi) cmd += "-framework Cocoa -framework CoreGraphics -framework Metal -framework QuartzCore ";
        cmd += "-o \"" + binPath.string() + "\"";
        if (verbose) std::cout << "[package:mac] " << cmd << "\n";
        std::string out;
        if (detail::execCmd(cmd, out) != 0) {
            res.error = "macOS build failed:\n" + out;
            return res;
        }

        std::string plist = R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
  <key>CFBundleDevelopmentRegion</key><string>en</string>
  <key>CFBundleExecutable</key><string>)" + appName + R"(</string>
  <key>CFBundleIdentifier</key><string>dev.memelang.)" + pkgName + R"(</string>
  <key>CFBundleInfoDictionaryVersion</key><string>6.0</string>
  <key>CFBundleName</key><string>)" + appName + R"(</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleShortVersionString</key><string>1.0.0</string>
  <key>CFBundleVersion</key><string>1</string>
  <key>NSHighResolutionCapable</key><true/>
</dict></plist>)";
        detail::writeFileText(appBundle / "Contents" / "Info.plist", plist);
        detail::writeFileText(appBundle / "Contents" / "PkgInfo", "APPL????");

        std::string signOut;
        detail::execCmd("codesign --force --sign - \"" + appBundle.string() + "\"", signOut);
        return finishOk(appBundle.string(), {appBundle.string(), binPath.string()});
    }

    if (target == "win" || target == "windows") {
        std::string zig = detail::findInPathOrPaths("zig", {"/opt/homebrew/bin/zig", "/usr/local/bin/zig"});
        if (zig.empty()) {
            res.error = "zig compiler not found for Windows cross-compilation";
            return res;
        }
        std::filesystem::path exePath = outBase / (pkgName + ".exe");
        std::string cmd = "\"" + zig + "\" c++ -std=c++20 -O3 -DNDEBUG -DMEME_NO_DEPTH_GUARD "
                          "-target x86_64-windows-gnu -I\"" + repoRoot.string() + "\" "
                          "\"" + cppPath.string() + "\" -luser32 -lgdi32 -ld2d1 -ldwrite -lws2_32 -lole32 "
                          "-o \"" + exePath.string() + "\"";
        if (verbose) std::cout << "[package:win] " << cmd << "\n";
        std::string out;
        if (detail::execCmd(cmd, out) != 0) {
            res.error = "Windows PE32+ build failed:\n" + out;
            return res;
        }
        return finishOk(exePath.string(), {exePath.string()});
    }

    if (target == "deb") {
        std::filesystem::path elfPath = workDir / (pkgName + "_linux_x86_64");
        std::string err;
        if (!detail::buildLinuxElf(repoRoot, cppPath, elfPath, err, verbose)) {
            res.error = err;
            return res;
        }
        auto elfBytes = detail::readFileBytes(elfPath);
        std::string desktop = "[Desktop Entry]\nType=Application\nName=" + appName +
                              "\nExec=/usr/local/bin/" + pkgName + "\nCategories=Utility;\n";
        std::vector<ArchiveFileEntry> dataEntries = {
            {"./usr/", {}, 0755, true},
            {"./usr/local/", {}, 0755, true},
            {"./usr/local/bin/", {}, 0755, true},
            {"./usr/local/bin/" + pkgName, elfBytes, 0755, false},
            {"./usr/share/", {}, 0755, true},
            {"./usr/share/applications/", {}, 0755, true},
            {"./usr/share/applications/" + pkgName + ".desktop", detail::strToBytes(desktop), 0644, false},
        };
        auto dataGz = gzipBytes(buildTar(dataEntries));

        size_t kb = std::max<size_t>(1, (elfBytes.size() + desktop.size()) / 1024);
        std::string control = "Package: " + pkgName + "\nVersion: 1.0.0\nArchitecture: amd64\n"
                              "Maintainer: MEMElang <dev@memelang.dev>\nInstalled-Size: " + std::to_string(kb) +
                              "\nSection: utils\nPriority: optional\nDescription: " + appName +
                              " built natively with MEME-UI\n";
        auto ctrlGz = gzipBytes(buildTar({{"./control", detail::strToBytes(control), 0644, false}}));
        auto arBytes = buildArArchive({
            {"debian-binary", detail::strToBytes("2.0\n")},
            {"control.tar.gz", ctrlGz},
            {"data.tar.gz", dataGz},
        });
        std::filesystem::path debPath = outBase / (pkgName + "_1.0.0_amd64.deb");
        detail::writeFileBytes(debPath, arBytes);
        return finishOk(debPath.string(), {debPath.string()});
    }

    if (target == "rpm") {
        std::filesystem::path elfPath = workDir / (pkgName + "_linux_x86_64");
        std::string err;
        if (!detail::buildLinuxElf(repoRoot, cppPath, elfPath, err, verbose)) {
            res.error = err;
            return res;
        }
        auto elfBytes = detail::readFileBytes(elfPath);
        std::string desktop = "[Desktop Entry]\nType=Application\nName=" + appName +
                              "\nExec=/usr/local/bin/" + pkgName + "\nCategories=Utility;\n";
        std::vector<ArchiveFileEntry> files = {
            {"/usr/local/bin/" + pkgName, elfBytes, 0755, false},
            {"/usr/share/applications/" + pkgName + ".desktop", detail::strToBytes(desktop), 0644, false},
        };
        auto rpmBytes = buildRpmPackage(pkgName, "1.0.0", "1", "x86_64", appName + " Native App", files);
        std::filesystem::path rpmPath = outBase / (pkgName + "-1.0.0-1.x86_64.rpm");
        detail::writeFileBytes(rpmPath, rpmBytes);
        return finishOk(rpmPath.string(), {rpmPath.string()});
    }

    if (target == "ipk") {
        std::filesystem::path elfPath = workDir / (pkgName + "_linux_x86_64");
        std::string err;
        if (!detail::buildLinuxElf(repoRoot, cppPath, elfPath, err, verbose)) {
            res.error = err;
            return res;
        }
        auto elfBytes = detail::readFileBytes(elfPath);
        std::vector<ArchiveFileEntry> dataEntries = {
            {"./usr/", {}, 0755, true},
            {"./usr/bin/", {}, 0755, true},
            {"./usr/bin/" + pkgName, elfBytes, 0755, false},
        };
        auto dataGz = gzipBytes(buildTar(dataEntries));
        size_t kb = std::max<size_t>(1, elfBytes.size() / 1024);
        std::string control = "Package: " + pkgName + "\nVersion: 1.0.0\nArchitecture: x86_64\n"
                              "Maintainer: MEMElang <dev@memelang.dev>\nInstalled-Size: " + std::to_string(kb) +
                              "\nSection: utils\nDescription: " + appName + " Embedded MEME-UI Native App\n";
        auto ctrlGz = gzipBytes(buildTar({{"./control", detail::strToBytes(control), 0644, false}}));
        auto ipkBytes = gzipBytes(buildTar({
            {"./debian-binary", detail::strToBytes("2.0\n"), 0644, false},
            {"./control.tar.gz", ctrlGz, 0644, false},
            {"./data.tar.gz", dataGz, 0644, false},
        }));
        std::filesystem::path ipkPath = outBase / (pkgName + "_1.0.0_x86_64.ipk");
        detail::writeFileBytes(ipkPath, ipkBytes);
        return finishOk(ipkPath.string(), {ipkPath.string()});
    }

    if (target == "apk") {
        auto sdk = detail::findAndroidSdk();
        if (sdk.empty()) {
            res.error = "Android SDK not found (set ANDROID_HOME)";
            return res;
        }
        auto ndk = detail::newestSubdir(sdk / "ndk");
        auto bt = detail::newestSubdir(sdk / "build-tools");
        auto platDir = detail::newestSubdir(sdk / "platforms");
        std::filesystem::path androidJar = platDir / "android.jar";
        std::filesystem::path toolchainBin = ndk / "toolchains" / "llvm" / "prebuilt" / "darwin-x86_64" / "bin";
        if (!std::filesystem::exists(toolchainBin)) {
            toolchainBin = ndk / "toolchains" / "llvm" / "prebuilt" / "linux-x86_64" / "bin";
        }
        std::filesystem::path ccArm = toolchainBin / "aarch64-linux-android24-clang++";
        std::filesystem::path ccX64 = toolchainBin / "x86_64-linux-android24-clang++";
        std::filesystem::path aapt2 = bt / "aapt2";
        std::filesystem::path zipalign = bt / "zipalign";
        std::filesystem::path apksigner = bt / "apksigner";
        if (!std::filesystem::exists(ccArm) || !std::filesystem::exists(aapt2) || !std::filesystem::exists(apksigner) || !std::filesystem::exists(androidJar)) {
            res.error = "Android SDK/NDK components missing in " + sdk.string();
            return res;
        }

        std::filesystem::path stage = workDir / "apk_stage";
        std::filesystem::remove_all(stage);
        std::filesystem::create_directories(stage / "lib" / "arm64-v8a");
        std::filesystem::create_directories(stage / "lib" / "x86_64");

        std::filesystem::path hostCpp = repoRoot / "src" / "hosts" / "android" / "MemeAndroidHost.cpp";
        auto buildAbi = [&](const std::filesystem::path &cc, const std::string &abi) -> bool {
            std::filesystem::path soOut = stage / "lib" / abi / "libmemeapp.so";
            std::string cmd = "\"" + cc.string() + "\" -std=c++20 -O3 -DNDEBUG -DMEME_NO_DEPTH_GUARD -DMEME_HOST_PROVIDES_MAIN "
                              "-shared -fPIC -I\"" + repoRoot.string() + "\" \"" + cppPath.string() + "\" "
                              "\"" + hostCpp.string() + "\" -landroid -llog -lEGL -lGLESv2 -o \"" + soOut.string() + "\"";
            if (verbose) std::cout << "[package:apk:" << abi << "] " << cmd << "\n";
            std::string out;
            if (detail::execCmd(cmd, out) != 0) {
                res.error = "Android NDK (" + abi + ") build failed:\n" + out;
                return false;
            }
            return true;
        };
        if (!buildAbi(ccArm, "arm64-v8a")) return res;
        if (std::filesystem::exists(ccX64) && !buildAbi(ccX64, "x86_64")) return res;

        std::string javaPkg = "dev.memelang." + detail::sanitizeJavaIdent(pkgName);
        std::string manifest = R"(<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package=")" + javaPkg + R"("
    android:versionCode="1"
    android:versionName="1.0.0">
    <uses-sdk android:minSdkVersion="24" android:targetSdkVersion="35" />
    <uses-permission android:name="android.permission.INTERNET" />
    <application android:label=")" + appName + R"(" android:hasCode="false" android:extractNativeLibs="false">
        <activity android:name="android.app.NativeActivity"
            android:label=")" + appName + R"("
            android:configChanges="orientation|keyboardHidden|screenSize|smallestScreenSize"
            android:exported="true">
            <meta-data android:name="android.app.lib_name" android:value="memeapp" />
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
    </application>
</manifest>
)";
        std::filesystem::path manifestPath = stage / "AndroidManifest.xml";
        detail::writeFileText(manifestPath, manifest);

        std::filesystem::path resApk = stage / "res.apk";
        std::filesystem::path alignedApk = stage / "aligned.apk";
        std::string out;
        if (detail::execCmd("\"" + aapt2.string() + "\" link -I \"" + androidJar.string() + "\" --manifest \"" +
                            manifestPath.string() + "\" -o \"" + resApk.string() + "\" --auto-add-overlay", out) != 0) {
            res.error = "aapt2 link failed:\n" + out;
            return res;
        }
        if (detail::execCmd("cd \"" + stage.string() + "\" && zip -0 -r \"" + resApk.string() + "\" lib", out) != 0) {
            res.error = "zip .so injection failed:\n" + out;
            return res;
        }
        if (detail::execCmd("\"" + zipalign.string() + "\" -f -p 4 \"" + resApk.string() + "\" \"" + alignedApk.string() + "\"", out) != 0) {
            res.error = "zipalign failed:\n" + out;
            return res;
        }

        std::filesystem::path ks = workDir / "meme_debug.keystore";
        if (!std::filesystem::exists(ks)) {
            std::string ktCmd = "keytool -genkeypair -noprompt -keystore \"" + ks.string() +
                                "\" -storepass android -keypass android -alias androiddebugkey "
                                "-dname \"CN=MEME-UI Debug,O=MEMElang,C=US\" -keyalg RSA -keysize 2048 -validity 10000";
            if (detail::execCmd(ktCmd, out) != 0) {
                res.error = "keytool keystore generation failed:\n" + out;
                return res;
            }
        }

        std::filesystem::path finalApk = outBase / (pkgName + ".apk");
        if (detail::execCmd("\"" + apksigner.string() + "\" sign --ks \"" + ks.string() +
                            "\" --ks-pass pass:android --key-pass pass:android --out \"" +
                            finalApk.string() + "\" \"" + alignedApk.string() + "\"", out) != 0) {
            res.error = "apksigner failed:\n" + out;
            return res;
        }
        return finishOk(finalApk.string(), {finalApk.string()});
    }

    if (target == "ipa" || target == "ios") {
        std::filesystem::path stage = workDir / "ipa_stage";
        std::filesystem::remove_all(stage);
        std::filesystem::path devApp = stage / "Payload" / (appName + ".app");
        std::filesystem::create_directories(devApp);
        std::filesystem::path hostMm = repoRoot / "src" / "hosts" / "ios" / "MemeIosHost.mm";

        auto writeIosPlist = [&](const std::filesystem::path &plistPath, const std::string &plat) {
            std::string pl = R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
  <key>CFBundleDevelopmentRegion</key><string>en</string>
  <key>CFBundleExecutable</key><string>)" + appName + R"(</string>
  <key>CFBundleIdentifier</key><string>dev.memelang.)" + pkgName + R"(</string>
  <key>CFBundleInfoDictionaryVersion</key><string>6.0</string>
  <key>CFBundleName</key><string>)" + appName + R"(</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleShortVersionString</key><string>1.0.0</string>
  <key>CFBundleVersion</key><string>1</string>
  <key>CFBundleSupportedPlatforms</key><array><string>)" + plat + R"(</string></array>
  <key>LSRequiresIPhoneOS</key><true/>
  <key>MinimumOSVersion</key><string>15.0</string>
  <key>UIDeviceFamily</key><array><integer>1</integer><integer>2</integer></array>
  <key>UIRequiredDeviceCapabilities</key><array><string>arm64</string></array>
  <key>UILaunchScreen</key><dict/>
  <key>UIApplicationSceneManifest</key>
  <dict>
    <key>UIApplicationSupportsMultipleScenes</key><false/>
    <key>UISceneConfigurations</key>
    <dict>
      <key>UIWindowSceneSessionRoleApplication</key>
      <array>
        <dict>
          <key>UISceneConfigurationName</key><string>Default Configuration</string>
          <key>UISceneDelegateClassName</key><string>MemeIosSceneDelegate</string>
        </dict>
      </array>
    </dict>
  </dict>
  <key>UIStatusBarStyle</key><string>UIStatusBarStyleLightContent</string>
  <key>UISupportedInterfaceOrientations</key><array>
    <string>UIInterfaceOrientationPortrait</string>
    <string>UIInterfaceOrientationLandscapeLeft</string>
    <string>UIInterfaceOrientationLandscapeRight</string>
  </array>
</dict></plist>)";
            detail::writeFileText(plistPath, pl);
        };

        std::filesystem::path devBin = devApp / appName;
        std::string cmdDev = "xcrun --sdk iphoneos clang++ -std=c++20 -O3 -DNDEBUG -DMEME_NO_DEPTH_GUARD "
                             "-arch arm64 -miphoneos-version-min=15.0 -fobjc-arc -DMEME_HOST_PROVIDES_MAIN "
                             "-I\"" + repoRoot.string() + "\" -x objective-c++ \"" + cppPath.string() + "\" "
                             "\"" + hostMm.string() + "\" -framework UIKit -framework Metal -framework QuartzCore -framework Foundation "
                             "-o \"" + devBin.string() + "\"";
        if (verbose) std::cout << "[package:ipa] " << cmdDev << "\n";
        std::string out;
        if (detail::execCmd(cmdDev, out) != 0) {
            res.error = "iOS device build failed:\n" + out;
            return res;
        }
        writeIosPlist(devApp / "Info.plist", "iPhoneOS");
        detail::execCmd("codesign --force --sign - \"" + devApp.string() + "\"", out);

        std::filesystem::path ipaPath = outBase / (appName + ".ipa");
        std::filesystem::remove(ipaPath);
        if (detail::execCmd("cd \"" + stage.string() + "\" && zip -qry \"" + std::filesystem::absolute(ipaPath).string() + "\" Payload", out) != 0) {
            res.error = "Failed to archive .ipa:\n" + out;
            return res;
        }

        // Also build iOS Simulator companion .app so xcrun simctl can boot/verify directly
        std::filesystem::path simApp = outBase / (appName + "-Simulator.app");
        std::filesystem::create_directories(simApp);
        std::filesystem::path simBin = simApp / appName;
        std::string simArch = "arm64";
#if defined(__x86_64__)
        simArch = "x86_64";
#endif
        std::string cmdSim = "xcrun --sdk iphonesimulator clang++ -std=c++20 -O3 -DNDEBUG -DMEME_NO_DEPTH_GUARD "
                             "-arch " + simArch + " -mios-simulator-version-min=15.0 -fobjc-arc -DMEME_HOST_PROVIDES_MAIN "
                             "-I\"" + repoRoot.string() + "\" -x objective-c++ \"" + cppPath.string() + "\" "
                             "\"" + hostMm.string() + "\" -framework UIKit -framework Metal -framework QuartzCore -framework Foundation "
                             "-o \"" + simBin.string() + "\"";
        if (detail::execCmd(cmdSim, out) == 0) {
            writeIosPlist(simApp / "Info.plist", "iPhoneSimulator");
            detail::execCmd("codesign --force --sign - \"" + simApp.string() + "\"", out);
        }
        return finishOk(ipaPath.string(), {ipaPath.string(), simApp.string()});
    }

    if (target == "web" || target == "wasm") {
        std::filesystem::path emppLocal = repoRoot / "tools" / "emsdk" / "upstream" / "emscripten" / "em++";
        std::string empp = detail::findInPathOrPaths("em++", {emppLocal.string()});
        if (empp.empty()) {
            res.error = "em++ (Emscripten) not found";
            return res;
        }
        std::filesystem::path webDir = outBase / "web";
        std::filesystem::create_directories(webDir);
        std::filesystem::path jsPath = webDir / (pkgName + ".js");
        std::filesystem::path htmlPath = webDir / "index.html";

        std::string envPrefix;
        std::filesystem::path emConfig = repoRoot / "tools" / "emsdk" / ".emscripten";
        if (std::filesystem::exists(emConfig)) {
            envPrefix = "EM_CONFIG=\"" + emConfig.string() + "\" ";
        }

        std::string exportedFns = "'[\"_meme_web_start\",\"_meme_web_render\",\"_meme_web_get_mesh_vertices\","
                                  "\"_meme_web_get_mesh_vertex_count\",\"_meme_web_get_mesh_indices\","
                                  "\"_meme_web_get_mesh_index_count\",\"_meme_web_get_batches_json\","
                                  "\"_meme_web_get_atlas_rgba\",\"_meme_web_get_atlas_width\",\"_meme_web_get_atlas_height\",\"_meme_web_get_commands_json\","
                                  "\"_meme_web_get_hit_regions_json\",\"_meme_web_get_agent_snapshot\","
                                  "\"_meme_web_pointer_move\",\"_meme_web_pointer_down\",\"_meme_web_pointer_up\","
                                  "\"_meme_web_scroll\",\"_meme_web_key_event\",\"_meme_web_tick\","
                                  "\"_meme_web_set_clipboard\",\"_meme_web_get_clipboard\",\"_meme_web_agent_act\","
                                  "\"_malloc\",\"_free\"]'";
        std::string cmd = envPrefix + "\"" + empp + "\" -std=c++20 -O3 -fexceptions -DMEME_HOST_PROVIDES_MAIN "
                          "-I\"" + repoRoot.string() + "\" -s WASM=1 -s EXPORTED_FUNCTIONS=" + exportedFns + " "
                          "-s EXPORTED_RUNTIME_METHODS='[\"ccall\",\"cwrap\",\"UTF8ToString\",\"stringToUTF8\",\"HEAPU8\",\"HEAPU32\"]' "
                          "-s STACK_SIZE=4194304 -s INITIAL_MEMORY=33554432 -s ALLOW_MEMORY_GROWTH=1 \"" + cppPath.string() + "\" -o \"" + jsPath.string() + "\"";
        if (verbose) std::cout << "[package:web] " << cmd << "\n";
        std::string out;
        if (detail::execCmd(cmd, out) != 0) {
            res.error = "Wasm web build failed:\n" + out;
            return res;
        }
        detail::writeFileText(htmlPath, MemeWebHost::generateZeroDomHtmlShell(pkgName + ".js", appName));
        std::filesystem::path wasmPath = webDir / (pkgName + ".wasm");
        return finishOk(htmlPath.string(), {htmlPath.string(), jsPath.string(), wasmPath.string()});
    }

    if (target == "all") {
        std::vector<std::string> targets = {"mac", "win", "deb", "rpm", "ipk", "apk", "ipa", "web"};
        std::vector<std::string> allArts;
        std::string errors;
        for (const auto &t : targets) {
            PackageResult sub = packageForTarget(sourceFile, t, outDirOrPath, verbose);
            if (!sub.success) {
                errors += "[" + t + "] " + sub.error + "\n";
            } else {
                allArts.insert(allArts.end(), sub.artifacts.begin(), sub.artifacts.end());
            }
        }
        if (!errors.empty()) {
            res.error = errors;
            res.artifacts = allArts;
            return res;
        }
        return finishOk(outBase.string(), allArts);
    }

    res.error = "Unknown package target '" + target + "'. Supported: mac, win, deb, rpm, ipk, apk, ipa, web, all";
    return res;
}

} // namespace MemePackager
