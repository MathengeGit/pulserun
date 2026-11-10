#include "core/storage.h"
#include <fstream>
#include <sstream>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace {
bool g_enabled = true;
const char* kFile = "pulserun_save.txt";
}  // namespace

void setStorageEnabled(bool enabled) { g_enabled = enabled; }

#ifdef __EMSCRIPTEN__
// clang-format off
// The save is plain ASCII, so bytes are copied by hand (no runtime string helpers needed).
EM_JS(int, pr_js_save_len, (), {
    try { var v = window.localStorage.getItem('pulserun.save'); return v === null ? 0 : v.length; }
    catch (e) { return 0; }
});
EM_JS(void, pr_js_save_read, (char* buf, int n), {
    try {
        var v = window.localStorage.getItem('pulserun.save') || '';
        for (var i = 0; i < n; i++) HEAPU8[buf + i] = v.charCodeAt(i) & 127;
    } catch (e) {}
});
EM_JS(void, pr_js_save_write, (const char* buf, int n), {
    try {
        var s = '';
        for (var i = 0; i < n; i++) s += String.fromCharCode(HEAPU8[buf + i]);
        window.localStorage.setItem('pulserun.save', s);
    } catch (e) {}
});
// clang-format on

std::string loadSaveText() {
    if (!g_enabled) return "";
    const int n = pr_js_save_len();
    std::string s((size_t)n, '\0');
    if (n > 0) pr_js_save_read(&s[0], n);
    return s;
}
void writeSaveText(const std::string& text) {
    if (g_enabled) pr_js_save_write(text.data(), (int)text.size());
}
#else
std::string loadSaveText() {
    if (!g_enabled) return "";
    std::ifstream f(kFile);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}
void writeSaveText(const std::string& text) {
    if (!g_enabled) return;
    std::ofstream f(kFile, std::ios::trunc);
    f << text;
}
#endif
