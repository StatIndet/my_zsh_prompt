#include "utils.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <sstream>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct DecodedChar {
        uint32_t codepoint;
        size_t next_index;
        bool valid;
};

struct LangDetect {
        std::string name;
        std::string icon;
        int weight;
        std::vector<std::string> triggers;
        std::vector<std::string> extensions;
};

struct LangMatch {
        LangDetect lang;
        int score;
};

struct GitStatus {
        std::string branch;
        std::string oid;
        bool detached = false;
        int ahead = 0;
        int behind = 0;
        int staged = 0;
        int modified = 0;
        int deleted = 0;
        int renamed = 0;
        int untracked = 0;
        int conflicted = 0;
        std::string operation;
};

bool starts_with(const std::string &value, const std::string &prefix) {
        return value.rfind(prefix, 0) == 0;
}

std::string trim_newlines(std::string value) {
        while (!value.empty() && (value.back() == '\n' || value.back() == '\r'))
                value.pop_back();
        return value;
}

std::vector<std::string> split_lines(const std::string &value) {
        std::vector<std::string> lines;
        std::stringstream stream(value);
        std::string line;
        while (std::getline(stream, line)) {
                if (!line.empty() && line.back() == '\r')
                        line.pop_back();
                lines.push_back(line);
        }
        return lines;
}

DecodedChar decode_utf8_at(const std::string &text, size_t index) {
        const unsigned char first = static_cast<unsigned char>(text[index]);
        if (first < 0x80)
                return {first, index + 1, true};

        int length = 0;
        uint32_t codepoint = 0;
        if ((first & 0xE0) == 0xC0) {
                length = 2;
                codepoint = first & 0x1F;
        } else if ((first & 0xF0) == 0xE0) {
                length = 3;
                codepoint = first & 0x0F;
        } else if ((first & 0xF8) == 0xF0) {
                length = 4;
                codepoint = first & 0x07;
        } else {
                return {first, index + 1, false};
        }

        if (index + static_cast<size_t>(length) > text.size())
                return {first, index + 1, false};

        for (int i = 1; i < length; ++i) {
                const unsigned char current =
                        static_cast<unsigned char>(text[index + i]);
                if ((current & 0xC0) != 0x80)
                        return {first, index + 1, false};
                codepoint = (codepoint << 6) | (current & 0x3F);
        }

        return {codepoint, index + static_cast<size_t>(length), true};
}

bool is_combining_codepoint(uint32_t codepoint) {
        return (codepoint >= 0x0300 && codepoint <= 0x036F) ||
               (codepoint >= 0x0483 && codepoint <= 0x0489) ||
               (codepoint >= 0x0591 && codepoint <= 0x05BD) ||
               (codepoint >= 0x05BF && codepoint <= 0x05BF) ||
               (codepoint >= 0x05C1 && codepoint <= 0x05C2) ||
               (codepoint >= 0x05C4 && codepoint <= 0x05C5) ||
               (codepoint >= 0x0610 && codepoint <= 0x061A) ||
               (codepoint >= 0x064B && codepoint <= 0x065F) ||
               (codepoint >= 0x0670 && codepoint <= 0x0670) ||
               (codepoint >= 0x06D6 && codepoint <= 0x06DC) ||
               (codepoint >= 0x06DF && codepoint <= 0x06E4) ||
               (codepoint >= 0x06E7 && codepoint <= 0x06E8) ||
               (codepoint >= 0x06EA && codepoint <= 0x06ED) ||
               (codepoint >= 0x0711 && codepoint <= 0x0711) ||
               (codepoint >= 0x0730 && codepoint <= 0x074A) ||
               (codepoint >= 0x07A6 && codepoint <= 0x07B0) ||
               (codepoint >= 0x07EB && codepoint <= 0x07F3) ||
               (codepoint >= 0x0816 && codepoint <= 0x0819) ||
               (codepoint >= 0x081B && codepoint <= 0x0823) ||
               (codepoint >= 0x0825 && codepoint <= 0x0827) ||
               (codepoint >= 0x0829 && codepoint <= 0x082D) ||
               (codepoint >= 0x0859 && codepoint <= 0x085B) ||
               (codepoint >= 0x08D3 && codepoint <= 0x08E1) ||
               (codepoint >= 0x08E3 && codepoint <= 0x0903) ||
               (codepoint >= 0x093A && codepoint <= 0x093C) ||
               (codepoint >= 0x0941 && codepoint <= 0x0948) ||
               (codepoint >= 0x094D && codepoint <= 0x094D) ||
               (codepoint >= 0x0951 && codepoint <= 0x0957) ||
               (codepoint >= 0x0962 && codepoint <= 0x0963) ||
               (codepoint >= 0x0981 && codepoint <= 0x0981) ||
               (codepoint >= 0x09BC && codepoint <= 0x09BC) ||
               (codepoint >= 0x09C1 && codepoint <= 0x09C4) ||
               (codepoint >= 0x09CD && codepoint <= 0x09CD) ||
               (codepoint >= 0x09E2 && codepoint <= 0x09E3) ||
               (codepoint >= 0x0A01 && codepoint <= 0x0A02) ||
               (codepoint >= 0x0A3C && codepoint <= 0x0A3C) ||
               (codepoint >= 0x0A41 && codepoint <= 0x0A42) ||
               (codepoint >= 0x0A47 && codepoint <= 0x0A48) ||
               (codepoint >= 0x0A4B && codepoint <= 0x0A4D) ||
               (codepoint >= 0x0A51 && codepoint <= 0x0A51) ||
               (codepoint >= 0x0A70 && codepoint <= 0x0A71) ||
               (codepoint >= 0x0A75 && codepoint <= 0x0A75) ||
               (codepoint >= 0x0B01 && codepoint <= 0x0B01) ||
               (codepoint >= 0x0B3C && codepoint <= 0x0B3C) ||
               (codepoint >= 0x0B3F && codepoint <= 0x0B3F) ||
               (codepoint >= 0x0B41 && codepoint <= 0x0B44) ||
               (codepoint >= 0x0B4D && codepoint <= 0x0B4D) ||
               (codepoint >= 0x0B56 && codepoint <= 0x0B56) ||
               (codepoint >= 0x0B62 && codepoint <= 0x0B63) ||
               (codepoint >= 0x0B82 && codepoint <= 0x0B82) ||
               (codepoint >= 0x0BC0 && codepoint <= 0x0BC0) ||
               (codepoint >= 0x0BCD && codepoint <= 0x0BCD) ||
               (codepoint >= 0x0C00 && codepoint <= 0x0C00) ||
               (codepoint >= 0x0C3E && codepoint <= 0x0C40) ||
               (codepoint >= 0x0C46 && codepoint <= 0x0C48) ||
               (codepoint >= 0x0C4A && codepoint <= 0x0C4D) ||
               (codepoint >= 0x0C55 && codepoint <= 0x0C56) ||
               (codepoint >= 0x0C62 && codepoint <= 0x0C63) ||
               (codepoint >= 0x0C81 && codepoint <= 0x0C81) ||
               (codepoint >= 0x0CBC && codepoint <= 0x0CBC) ||
               (codepoint >= 0x0CBF && codepoint <= 0x0CBF) ||
               (codepoint >= 0x0CC6 && codepoint <= 0x0CC6) ||
               (codepoint >= 0x0CCC && codepoint <= 0x0CCD) ||
               (codepoint >= 0x0CE2 && codepoint <= 0x0CE3) ||
               (codepoint >= 0x0D00 && codepoint <= 0x0D01) ||
               (codepoint >= 0x0D3B && codepoint <= 0x0D3C) ||
               (codepoint >= 0x0D41 && codepoint <= 0x0D44) ||
               (codepoint >= 0x0D4D && codepoint <= 0x0D4D) ||
               (codepoint >= 0x0D62 && codepoint <= 0x0D63) ||
               (codepoint >= 0x0DCA && codepoint <= 0x0DCA) ||
               (codepoint >= 0x0DD2 && codepoint <= 0x0DD4) ||
               (codepoint >= 0x0DD6 && codepoint <= 0x0DD6) ||
               (codepoint >= 0x0E31 && codepoint <= 0x0E31) ||
               (codepoint >= 0x0E34 && codepoint <= 0x0E3A) ||
               (codepoint >= 0x0E47 && codepoint <= 0x0E4E) ||
               (codepoint >= 0x0EB1 && codepoint <= 0x0EB1) ||
               (codepoint >= 0x0EB4 && codepoint <= 0x0EB9) ||
               (codepoint >= 0x0EBB && codepoint <= 0x0EBC) ||
               (codepoint >= 0x0EC8 && codepoint <= 0x0ECD) ||
               (codepoint >= 0x0F18 && codepoint <= 0x0F19) ||
               (codepoint >= 0x0F35 && codepoint <= 0x0F35) ||
               (codepoint >= 0x0F37 && codepoint <= 0x0F37) ||
               (codepoint >= 0x0F39 && codepoint <= 0x0F39) ||
               (codepoint >= 0x0F71 && codepoint <= 0x0F7E) ||
               (codepoint >= 0x0F80 && codepoint <= 0x0F84) ||
               (codepoint >= 0x0F86 && codepoint <= 0x0F87) ||
               (codepoint >= 0x0F8D && codepoint <= 0x0F97) ||
               (codepoint >= 0x0F99 && codepoint <= 0x0FBC) ||
               (codepoint >= 0x0FC6 && codepoint <= 0x0FC6) ||
               (codepoint >= 0x102D && codepoint <= 0x1030) ||
               (codepoint >= 0x1032 && codepoint <= 0x1037) ||
               (codepoint >= 0x1039 && codepoint <= 0x103A) ||
               (codepoint >= 0x103D && codepoint <= 0x103E) ||
               (codepoint >= 0x1058 && codepoint <= 0x1059) ||
               (codepoint >= 0x105E && codepoint <= 0x1060) ||
               (codepoint >= 0x1071 && codepoint <= 0x1074) ||
               (codepoint >= 0x1082 && codepoint <= 0x1082) ||
               (codepoint >= 0x1085 && codepoint <= 0x1086) ||
               (codepoint >= 0x108D && codepoint <= 0x108D) ||
               (codepoint >= 0x109D && codepoint <= 0x109D) ||
               (codepoint >= 0x135D && codepoint <= 0x135F) ||
               (codepoint >= 0x1712 && codepoint <= 0x1714) ||
               (codepoint >= 0x1732 && codepoint <= 0x1734) ||
               (codepoint >= 0x1752 && codepoint <= 0x1753) ||
               (codepoint >= 0x1772 && codepoint <= 0x1773) ||
               (codepoint >= 0x17B4 && codepoint <= 0x17B5) ||
               (codepoint >= 0x17B7 && codepoint <= 0x17BD) ||
               (codepoint >= 0x17C6 && codepoint <= 0x17C6) ||
               (codepoint >= 0x17C9 && codepoint <= 0x17D3) ||
               (codepoint >= 0x17DD && codepoint <= 0x17DD) ||
               (codepoint >= 0x180B && codepoint <= 0x180D) ||
               (codepoint >= 0x1885 && codepoint <= 0x1886) ||
               (codepoint >= 0x18A9 && codepoint <= 0x18A9) ||
               (codepoint >= 0x1920 && codepoint <= 0x1922) ||
               (codepoint >= 0x1927 && codepoint <= 0x1928) ||
               (codepoint >= 0x1932 && codepoint <= 0x1932) ||
               (codepoint >= 0x1939 && codepoint <= 0x193B) ||
               (codepoint >= 0x1A17 && codepoint <= 0x1A18) ||
               (codepoint >= 0x1A1B && codepoint <= 0x1A1B) ||
               (codepoint >= 0x1A56 && codepoint <= 0x1A56) ||
               (codepoint >= 0x1A58 && codepoint <= 0x1A5E) ||
               (codepoint >= 0x1A60 && codepoint <= 0x1A60) ||
               (codepoint >= 0x1A62 && codepoint <= 0x1A62) ||
               (codepoint >= 0x1A65 && codepoint <= 0x1A6C) ||
               (codepoint >= 0x1A73 && codepoint <= 0x1A7C) ||
               (codepoint >= 0x1A7F && codepoint <= 0x1A7F) ||
               (codepoint >= 0x1AB0 && codepoint <= 0x1AFF) ||
               (codepoint >= 0x1B00 && codepoint <= 0x1B03) ||
               (codepoint >= 0x1B34 && codepoint <= 0x1B34) ||
               (codepoint >= 0x1B36 && codepoint <= 0x1B3A) ||
               (codepoint >= 0x1B3C && codepoint <= 0x1B3C) ||
               (codepoint >= 0x1B42 && codepoint <= 0x1B42) ||
               (codepoint >= 0x1B6B && codepoint <= 0x1B73) ||
               (codepoint >= 0x1B80 && codepoint <= 0x1B81) ||
               (codepoint >= 0x1BA2 && codepoint <= 0x1BA5) ||
               (codepoint >= 0x1BA8 && codepoint <= 0x1BA9) ||
               (codepoint >= 0x1BAB && codepoint <= 0x1BAD) ||
               (codepoint >= 0x1BE6 && codepoint <= 0x1BE6) ||
               (codepoint >= 0x1BE8 && codepoint <= 0x1BE9) ||
               (codepoint >= 0x1BED && codepoint <= 0x1BED) ||
               (codepoint >= 0x1BEF && codepoint <= 0x1BF1) ||
               (codepoint >= 0x1C2C && codepoint <= 0x1C33) ||
               (codepoint >= 0x1C36 && codepoint <= 0x1C37) ||
               (codepoint >= 0x1CD0 && codepoint <= 0x1CD2) ||
               (codepoint >= 0x1CD4 && codepoint <= 0x1CE0) ||
               (codepoint >= 0x1CE2 && codepoint <= 0x1CE8) ||
               (codepoint >= 0x1CED && codepoint <= 0x1CED) ||
               (codepoint >= 0x1CF4 && codepoint <= 0x1CF4) ||
               (codepoint >= 0x1CF8 && codepoint <= 0x1CF9) ||
               (codepoint >= 0x1DC0 && codepoint <= 0x1DFF) ||
               (codepoint >= 0x200B && codepoint <= 0x200F) ||
               (codepoint >= 0x202A && codepoint <= 0x202E) ||
               (codepoint >= 0x2060 && codepoint <= 0x206F) ||
               (codepoint >= 0x20D0 && codepoint <= 0x20FF) ||
               (codepoint >= 0xFE00 && codepoint <= 0xFE0F) ||
               (codepoint >= 0xFE20 && codepoint <= 0xFE2F) ||
               (codepoint >= 0xE0100 && codepoint <= 0xE01EF);
}

bool is_wide_codepoint(uint32_t codepoint) {
        return (codepoint >= 0x1100 &&
                (codepoint <= 0x115F || codepoint == 0x2329 ||
                        codepoint == 0x232A ||
                        (codepoint >= 0x2E80 && codepoint <= 0xA4CF &&
                                codepoint != 0x303F) ||
                        (codepoint >= 0xAC00 && codepoint <= 0xD7A3) ||
                        (codepoint >= 0xF900 && codepoint <= 0xFAFF) ||
                        (codepoint >= 0xFE10 && codepoint <= 0xFE19) ||
                        (codepoint >= 0xFE30 && codepoint <= 0xFE6F) ||
                        (codepoint >= 0xFF00 && codepoint <= 0xFF60) ||
                        (codepoint >= 0xFFE0 && codepoint <= 0xFFE6) ||
                        (codepoint >= 0x1F300 && codepoint <= 0x1FAFF))) ||
               (codepoint >= 0x2600 && codepoint <= 0x27BF);
}

int codepoint_width(uint32_t codepoint) {
        if (codepoint == 0 || codepoint < 32 ||
                        (codepoint >= 0x7F && codepoint < 0xA0) ||
                        codepoint == 0x200D || is_combining_codepoint(codepoint))
                return 0;
        return is_wide_codepoint(codepoint) ? 2 : 1;
}

size_t skip_ansi_escape(const std::string &text, size_t index) {
        if (text[index] != '\033' || index + 1 >= text.size())
                return index;

        if (text[index + 1] == '[') {
                size_t pos = index + 2;
                while (pos < text.size()) {
                        const unsigned char ch = static_cast<unsigned char>(text[pos]);
                        if (ch >= 0x40 && ch <= 0x7E)
                                return pos + 1;
                        ++pos;
                }
                return text.size();
        }

        if (text[index + 1] == ']') {
                size_t pos = index + 2;
                while (pos < text.size()) {
                        if (text[pos] == '\a')
                                return pos + 1;
                        if (text[pos] == '\033' && pos + 1 < text.size() &&
                                        text[pos + 1] == '\\')
                                return pos + 2;
                        ++pos;
                }
                return text.size();
        }

        return index;
}

int strftime_token_width(char token) {
        switch (token) {
        case 'H':
        case 'I':
        case 'M':
        case 'S':
        case 'd':
        case 'm':
        case 'y':
                return 2;
        case 'Y':
                return 4;
        case 'j':
                return 3;
        case 'p':
        case 'P':
                return 2;
        case '%':
                return 1;
        default:
                return 0;
        }
}

int strftime_visible_width(const std::string &format) {
        int width = 0;
        for (size_t i = 0; i < format.size();) {
                if (format[i] == '%' && i + 1 < format.size()) {
                        width += strftime_token_width(format[i + 1]);
                        i += 2;
                        continue;
                }

                DecodedChar decoded = decode_utf8_at(format, i);
                width += codepoint_width(decoded.codepoint);
                i = decoded.next_index;
        }
        return width;
}

size_t skip_zsh_prompt_escape(const std::string &text, size_t index, int &width) {
        if (text[index] != '%' || index + 1 >= text.size())
                return index;

        const char token = text[index + 1];
        if (token == '%') {
                width += 1;
                return index + 2;
        }

        if ((token == 'F' || token == 'K') && index + 2 < text.size() &&
                        text[index + 2] == '{') {
                size_t end = text.find('}', index + 3);
                return end == std::string::npos ? text.size() : end + 1;
        }

        if (token == 'f' || token == 'k' || token == 'b' || token == 'B' ||
                        token == 'u' || token == 'U' || token == 's' ||
                        token == 'S')
                return index + 2;

        if (token == 'D' && index + 2 < text.size() && text[index + 2] == '{') {
                size_t end = text.find('}', index + 3);
                if (end == std::string::npos)
                        return text.size();
                width += strftime_visible_width(text.substr(index + 3, end - index - 3));
                return end + 1;
        }

        return index;
}

std::string truncate_visible(const std::string &text, int max_width) {
        if (visible_width(text) <= max_width)
                return text;

        const std::string suffix = "..";
        const int suffix_width = visible_width(suffix);
        const int target_width = std::max(0, max_width - suffix_width);

        std::string result;
        int width = 0;
        for (size_t i = 0; i < text.size();) {
                DecodedChar decoded = decode_utf8_at(text, i);
                const int next_width = codepoint_width(decoded.codepoint);
                if (width + next_width > target_width)
                        break;
                result.append(text.substr(i, decoded.next_index - i));
                width += next_width;
                i = decoded.next_index;
        }

        return result + suffix;
}

bool status_code_changed(char code) {
        return code != '.' && code != ' ';
}

int parse_signed_count(const std::string &value) {
        if (value.empty())
                return 0;
        size_t start = (value[0] == '+' || value[0] == '-') ? 1 : 0;
        try {
                return std::stoi(value.substr(start));
        } catch (...) {
                return 0;
        }
}

GitStatus parse_git_status_v2(const std::string &git_status) {
        GitStatus status;
        for (const auto &line : split_lines(git_status)) {
                if (starts_with(line, "# branch.oid ")) {
                        status.oid = line.substr(std::string("# branch.oid ").size());
                } else if (starts_with(line, "# branch.head ")) {
                        status.branch = line.substr(std::string("# branch.head ").size());
                        status.detached = status.branch == "(detached)";
                } else if (starts_with(line, "# branch.ab ")) {
                        std::stringstream stream(
                                line.substr(std::string("# branch.ab ").size()));
                        std::string ahead_token, behind_token;
                        stream >> ahead_token >> behind_token;
                        status.ahead = parse_signed_count(ahead_token);
                        status.behind = parse_signed_count(behind_token);
                } else if (starts_with(line, "1 ") && line.size() >= 4) {
                        const char staged = line[2];
                        const char worktree = line[3];
                        if (status_code_changed(staged))
                                status.staged++;
                        if (worktree == 'M' || worktree == 'T')
                                status.modified++;
                        if (staged == 'D' || worktree == 'D')
                                status.deleted++;
                } else if (starts_with(line, "2 ") && line.size() >= 4) {
                        const char staged = line[2];
                        const char worktree = line[3];
                        if (status_code_changed(staged))
                                status.staged++;
                        if (worktree == 'M' || worktree == 'T')
                                status.modified++;
                        if (staged == 'D' || worktree == 'D')
                                status.deleted++;
                        status.renamed++;
                } else if (starts_with(line, "u ")) {
                        status.conflicted++;
                } else if (starts_with(line, "? ")) {
                        status.untracked++;
                }
        }
        return status;
}

std::string short_oid(const std::string &oid) {
        if (oid.empty() || oid == "(initial)")
                return "";
        return oid.substr(0, std::min<size_t>(7, oid.size()));
}

std::string detect_git_operation() {
        std::string git_dir =
                trim_newlines(exec_cmd("timeout 0.2s git rev-parse --git-dir 2>/dev/null"));
        if (git_dir.empty())
                return "";

        fs::path path = git_dir;
        try {
                if (fs::exists(path / "rebase-merge") || fs::exists(path / "rebase-apply"))
                        return "REBASE";
                if (fs::exists(path / "MERGE_HEAD"))
                        return "MERGE";
                if (fs::exists(path / "CHERRY_PICK_HEAD"))
                        return "PICK";
                if (fs::exists(path / "REVERT_HEAD"))
                        return "REVERT";
        } catch (...) {
        }

        return "";
}

std::string format_git_sync(const GitStatus &status) {
        if (status.ahead > 0 && status.behind > 0)
                return "⇕" + std::to_string(status.ahead) + "/" +
                       std::to_string(status.behind);
        if (status.ahead > 0)
                return "⇡" + std::to_string(status.ahead);
        if (status.behind > 0)
                return "⇣" + std::to_string(status.behind);
        return "";
}

std::vector<std::string> format_git_changes(const GitStatus &status) {
        std::vector<std::string> changes;
        if (status.conflicted > 0)
                changes.push_back("!" + std::to_string(status.conflicted));
        if (status.staged > 0)
                changes.push_back("+" + std::to_string(status.staged));
        if (status.modified > 0)
                changes.push_back("~" + std::to_string(status.modified));
        if (status.deleted > 0)
                changes.push_back("-" + std::to_string(status.deleted));
        if (status.renamed > 0)
                changes.push_back("»" + std::to_string(status.renamed));
        if (status.untracked > 0)
                changes.push_back("?" + std::to_string(status.untracked));

        const size_t max_changes = 4;
        if (changes.size() > max_changes)
                changes.resize(max_changes);
        return changes;
}

} // namespace

std::string exec_cmd(const char *cmd) {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, int (*)(FILE *)> pipe(popen(cmd, "r"), pclose);
        if (!pipe)
                return "";
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                result += buffer.data();
        }
        return result;
}

int visible_width(const std::string &text) {
        int width = 0;
        for (size_t i = 0; i < text.size();) {
                if (text[i] == '\033') {
                        size_t next = skip_ansi_escape(text, i);
                        if (next != i) {
                                i = next;
                                continue;
                        }
                }

                if (text[i] == '%') {
                        size_t next = skip_zsh_prompt_escape(text, i, width);
                        if (next != i) {
                                i = next;
                                continue;
                        }
                }

                DecodedChar decoded = decode_utf8_at(text, i);
                width += codepoint_width(decoded.codepoint);
                i = decoded.next_index;
        }
        return width;
}

RenderedPill render_text(const std::string &color, const std::string &text) {
        return {"%F{" + color + "}" + text + "%f", visible_width(text)};
}

RenderedPill render_pill(const std::string &color, const std::string &inner_text) {
        return render_pill(color, inner_text, inner_text);
}

RenderedPill render_pill(const std::string &color, const std::string &inner_zsh_code,
                const std::string &visible_text) {
        std::string zsh_code = "%F{" + color + "}%K{" + color + "}%F{" + C_BASE +
                "}" + inner_zsh_code + "%k%F{" + color + "}%f";
        return {zsh_code, visible_width("" + visible_text + "")};
}

RenderedPill get_lang_env() {
        std::vector<LangDetect> languages = {
                {"Python",
                        "󰌠",
                        100,
                        {"requirements.txt", "pyproject.toml", "setup.py", "Pipfile"},
                        {".py"}},
                {"Go", "", 95, {"go.mod"}, {".go"}},
                {"C++",
                        "",
                        90,
                        {"CMakeLists.txt", "meson.build"},
                        {".cpp", ".hpp", ".cc", ".cxx"}},
                {"TypeScript", "", 88, {"tsconfig.json"}, {".ts", ".tsx"}},
                {"JavaScript",
                        "",
                        86,
                        {"package.json"},
                        {".js", ".jsx", ".mjs", ".cjs"}},
                {"Java", "", 85, {"pom.xml", "build.gradle"}, {".java"}},
                {"C", "", 80, {}, {".c", ".h"}},
                {"QML", "", 78, {"qmldir"}, {".qml"}},
                {"Rust", "", 76, {"Cargo.toml"}, {".rs"}},
                {"Vue", "", 74, {}, {".vue"}},
                {"Svelte", "", 72, {}, {".svelte"}},
                {"Zig", "", 70, {"build.zig"}, {".zig"}},
                {"Lua", "", 65, {".luarc.json"}, {".lua"}},
                {"Shell", "", 62, {}, {".sh", ".bash", ".zsh"}},
                {"Docker",
                        "",
                        55,
                        {"Dockerfile", "docker-compose.yml", "docker-compose.yaml",
                                "compose.yml", "compose.yaml"},
                        {}},
                {"Nix", "", 54, {"flake.nix", "shell.nix", "default.nix"}, {".nix"}},
                {"CMake", "", 52, {"CMakeLists.txt"}, {".cmake"}},
                {"Meson", "󰔷", 50, {"meson.build"}, {}},
                {"Just", "", 45, {"Justfile", "justfile", ".justfile"}, {".just"}}};

        std::unordered_set<std::string> dir_files;
        std::unordered_set<std::string> dir_exts;

        try {
                int count = 0;
                for (const auto &entry : fs::directory_iterator(fs::current_path())) {
                        if (count++ > 1000)
                                break;
                        dir_files.insert(entry.path().filename().string());
                        dir_exts.insert(entry.path().extension().string());
                }
        } catch (...) {
        }

        std::vector<LangMatch> found_langs;
        for (const auto &lang : languages) {
                bool trigger_match = false;
                bool extension_match = false;

                for (const auto &trigger : lang.triggers) {
                        if (dir_files.count(trigger)) {
                                trigger_match = true;
                                break;
                        }
                }

                for (const auto &ext : lang.extensions) {
                        if (dir_exts.count(ext)) {
                                extension_match = true;
                                break;
                        }
                }

                if (trigger_match || extension_match) {
                        int score = lang.weight;
                        if (extension_match)
                                score += 100;
                        if (trigger_match)
                                score += 30;
                        found_langs.push_back({lang, score});
                }
        }

        if (found_langs.empty())
                return {"", 0};

        std::sort(found_langs.begin(), found_langs.end(),
                        [](const LangMatch &a, const LangMatch &b) {
                        if (a.score != b.score)
                                return a.score > b.score;
                        return a.lang.name < b.lang.name;
                        });

        std::vector<std::string> display_items;
        const size_t max_languages = 7;
        const size_t shown_count = std::min(max_languages, found_langs.size());
        for (size_t i = 0; i < shown_count; ++i) {
                display_items.push_back(found_langs[i].lang.icon);
        }
        if (found_langs.size() > max_languages) {
                display_items.push_back("+" + std::to_string(found_langs.size() - max_languages));
        }

        std::string inner_text = " ";
        for (size_t i = 0; i < display_items.size(); ++i) {
                inner_text += display_items[i];
                if (i + 1 < display_items.size())
                        inner_text += " ";
        }
        inner_text += " ";

        return render_pill(C_GREEN, inner_text);
}

RenderedPill get_directory_pill() {
        std::string pwd;
        try {
                pwd = fs::current_path().string();
        } catch (...) {
                pwd = "?";
        }

        std::string home = getenv("HOME") ? getenv("HOME") : "";
        if (!home.empty() && (pwd == home || starts_with(pwd, home + "/"))) {
                pwd.replace(0, home.length(), "~");
        }

        // 1. 定义已知目录前缀和它们对应的图标
        // (注意：我去掉了图标自带的空格，便于拼接)
        struct DirSub {
                std::string prefix;
                std::string icon;
        };
        std::vector<DirSub> subs = {
                {"~/Downloads", ""},  {"~/Desktop", ""},    {"~/Documents", ""},
                {"~/Pictures", ""},   {"~/Videos", ""},     {"~/Music", "󰎈"},
                {"~/Templates", "󰏪"}, {"~/Public", "󰙨"},    {"~/GitHub", "󰊤"},
                {"~/Projects", "󰊤"},  {"~/Workspace", "󰊤"}, {"~", ""}};

        std::string icon = "";
        std::string display_path = pwd;
        bool matched = false;

        // 2. 匹配并剔除冗余前缀文本
        for (const auto &sub : subs) {
                if (pwd == sub.prefix) {
                        icon = sub.icon;
                        display_path = ""; // 完全匹配，直接清空文本
                        matched = true;
                        break;
                } else if (pwd.find(sub.prefix + "/") == 0) {
                        icon = sub.icon;
                        // 截取掉前缀，只保留后续路径
                        display_path = pwd.substr(sub.prefix.length() + 1);
                        matched = true;
                        break;
                }
        }

        if (!matched && pwd == "/") {
                icon = "󰀘";
                display_path = "";
        }

        // 3. 截断逻辑 (最大显示 3 级目录，同款 end_4 的 ••/ 逻辑)
        const size_t max_components = 3;
        std::vector<std::string> parts;
        size_t start = 0, end;
        while ((end = display_path.find('/', start)) != std::string::npos) {
                if (end != start) {
                        parts.push_back(display_path.substr(start, end - start));
                }
                start = end + 1;
        }
        if (start < display_path.length()) {
                parts.push_back(display_path.substr(start));
        }

        std::string final_path = display_path;
        if (parts.size() > max_components) {
                final_path = "••/";
                // 只保留最后的 max_components 个目录
                for (size_t i = parts.size() - max_components; i < parts.size(); ++i) {
                        final_path += parts[i];
                        if (i != parts.size() - 1)
                                final_path += "/";
                }
        }

        // 4. 组装胶囊内部文本
        std::string inner_text = icon;

        // 如果后面还有路径，我们加上空格隔开
        if (!final_path.empty()) {
                inner_text += " " + final_path;
        }
        inner_text += " ";

        return render_pill(C_BLUE, inner_text);
}

RenderedPill get_git_pill() {
        std::string git_status = exec_cmd(
                "timeout 0.5s git -c core.quotepath=false status "
                "--porcelain=v2 --branch 2>/dev/null");
        if (git_status.empty())
                return {"", 0};

        GitStatus status = parse_git_status_v2(git_status);
        status.operation = detect_git_operation();

        std::string branch = status.branch.empty() ? "Git" : status.branch;
        if (status.detached) {
                std::string oid = short_oid(status.oid);
                branch = oid.empty() ? "detached" : "detached@" + oid;
        }
        branch = truncate_visible(branch, 16);

        std::string inner_text = "󰘬 " + branch;

        if (!status.operation.empty())
                inner_text += " " + status.operation;

        std::string sync = format_git_sync(status);
        if (!sync.empty())
                inner_text += " " + sync;

        for (const auto &change : format_git_changes(status)) {
                inner_text += " " + change;
        }
        inner_text += " ";

        return render_pill(C_PURPLE, inner_text);
}
