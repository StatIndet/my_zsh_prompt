#include "utils.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;

namespace {

struct ScopedCwd {
        fs::path old_path;

        explicit ScopedCwd(const fs::path &new_path) : old_path(fs::current_path()) {
                fs::current_path(new_path);
        }

        ~ScopedCwd() {
                try {
                        fs::current_path(old_path);
                } catch (...) {
                }
        }
};

void check(bool condition, const std::string &message) {
        if (!condition)
                throw std::runtime_error(message);
}

bool contains(const std::string &haystack, const std::string &needle) {
        return haystack.find(needle) != std::string::npos;
}

std::string shell_quote(const fs::path &path) {
        std::string raw = path.string();
        std::string quoted = "'";
        for (char ch : raw) {
                if (ch == '\'')
                        quoted += "'\\''";
                else
                        quoted += ch;
        }
        quoted += "'";
        return quoted;
}

void require_run(const std::string &command) {
        int code = std::system(command.c_str());
        if (code != 0)
                throw std::runtime_error("command failed: " + command);
}

void write_file(const fs::path &path, const std::string &content) {
        fs::create_directories(path.parent_path());
        std::ofstream file(path);
        if (!file)
                throw std::runtime_error("cannot write file: " + path.string());
        file << content;
}

fs::path make_temp_dir(const std::string &name) {
        fs::path path = fs::temp_directory_path() /
                ("prompt_dev_tests_" + std::to_string(getpid()) + "_" + name);
        fs::remove_all(path);
        fs::create_directories(path);
        return path;
}

void init_repo(const fs::path &repo) {
        fs::create_directories(repo);
        require_run("git init -b main " + shell_quote(repo) + " >/dev/null 2>&1");
        require_run("git -C " + shell_quote(repo) +
                " config user.email prompt-test@example.invalid");
        require_run("git -C " + shell_quote(repo) + " config user.name Prompt Test");
}

void commit_all(const fs::path &repo, const std::string &message) {
        require_run("git -C " + shell_quote(repo) + " add -A");
        require_run("git -C " + shell_quote(repo) + " commit -m " +
                shell_quote(message) + " >/dev/null 2>&1");
}

void test_visible_width() {
        check(visible_width("abc") == 3, "ascii width");
        check(visible_width("中文") == 4, "cjk width");
        check(visible_width("") == 1, "powerline width");
        check(visible_width("󰘬") == 1, "nerd font icon width");
        check(visible_width("😀") == 2, "emoji width");
        check(visible_width("%F{#fff}abc%f") == 3, "zsh style escape width");
        check(visible_width("  %D{%H:%M:%S} ") ==
                        visible_width("  00:00:00 "),
                "zsh time escape width");
}

void test_directory_paths() {
        fs::path long_dir = make_temp_dir("long_path") / "a" / "b" / "c" / "d";
        fs::create_directories(long_dir);
        {
                ScopedCwd cwd(long_dir);
                RenderedPill pill = get_directory_pill();
                check(contains(pill.zsh_code, "••/"), "long path truncation");
        }

        fs::path cjk_dir = make_temp_dir("cjk_path") / "中文路径";
        fs::create_directories(cjk_dir);
        {
                ScopedCwd cwd(cjk_dir);
                RenderedPill pill = get_directory_pill();
                check(contains(pill.zsh_code, "中文路径"), "cjk path display");
                check(visible_width("中文路径") == 8, "cjk path width");
        }
}

void test_language_detection() {
        fs::path multi = make_temp_dir("languages");
        write_file(multi / "main.qml", "");
        write_file(multi / "main.cpp", "");
        write_file(multi / "tool.py", "");
        write_file(multi / "script.sh", "");
        write_file(multi / "app.ts", "");
        {
                ScopedCwd cwd(multi);
                RenderedPill pill = get_lang_env();
                check(contains(pill.zsh_code, ""), "qml detected");
                check(contains(pill.zsh_code, ""), "cpp detected");
                check(contains(pill.zsh_code, "󰌠"), "python detected");
                check(contains(pill.zsh_code, ""), "shell detected");
                check(contains(pill.zsh_code, ""), "typescript detected");
                check(!contains(pill.zsh_code, ""), "typescript is not javascript");
        }

        fs::path package_only = make_temp_dir("package_only");
        write_file(package_only / "package.json", "{}\n");
        {
                ScopedCwd cwd(package_only);
                RenderedPill pill = get_lang_env();
                check(contains(pill.zsh_code, ""), "package marker detected");
                check(!contains(pill.zsh_code, ""), "package marker is not ts");
        }

        fs::path tools = make_temp_dir("tools");
        write_file(tools / "CMakeLists.txt", "");
        write_file(tools / "meson.build", "");
        write_file(tools / "Dockerfile", "");
        write_file(tools / "flake.nix", "");
        write_file(tools / "Justfile", "");
        write_file(tools / "component.svelte", "");
        write_file(tools / "view.vue", "");
        write_file(tools / "main.zig", "");
        {
                ScopedCwd cwd(tools);
                RenderedPill pill = get_lang_env();
                check(contains(pill.zsh_code, "+"), "language overflow indicator");
        }
}

void test_git_non_git_and_clean() {
        fs::path plain = make_temp_dir("nongit");
        {
                ScopedCwd cwd(plain);
                check(get_git_pill().visible_width == 0, "non-git hidden");
        }

        fs::path repo = make_temp_dir("git_clean");
        init_repo(repo);
        write_file(repo / "tracked.txt", "base\n");
        commit_all(repo, "initial");
        {
                ScopedCwd cwd(repo);
                RenderedPill pill = get_git_pill();
                check(contains(pill.zsh_code, "󰘬 main"), "clean branch displayed");
                check(!contains(pill.zsh_code, "~1"), "clean has no modified token");
        }
}

void test_git_dirty_changes() {
        fs::path repo = make_temp_dir("git_dirty");
        init_repo(repo);
        write_file(repo / "tracked.txt", "base\n");
        write_file(repo / "delete.txt", "delete\n");
        commit_all(repo, "initial");

        write_file(repo / "staged.txt", "staged\n");
        require_run("git -C " + shell_quote(repo) + " add staged.txt");
        write_file(repo / "tracked.txt", "modified\n");
        fs::remove(repo / "delete.txt");
        write_file(repo / "untracked.txt", "untracked\n");

        {
                ScopedCwd cwd(repo);
                RenderedPill pill = get_git_pill();
                check(contains(pill.zsh_code, "+1"), "staged token");
                check(contains(pill.zsh_code, "~1"), "modified token");
                check(contains(pill.zsh_code, "-1"), "deleted token");
                check(contains(pill.zsh_code, "?1"), "untracked token");
        }
}

void test_git_renamed() {
        fs::path repo = make_temp_dir("git_renamed");
        init_repo(repo);
        write_file(repo / "old.txt", "old\n");
        commit_all(repo, "initial");
        require_run("git -C " + shell_quote(repo) + " mv old.txt new.txt");

        {
                ScopedCwd cwd(repo);
                RenderedPill pill = get_git_pill();
                check(contains(pill.zsh_code, "+1"), "rename is staged");
                check(contains(pill.zsh_code, "»1"), "renamed token");
        }
}

void clone_and_config(const fs::path &remote, const fs::path &target) {
        require_run("git clone " + shell_quote(remote) + " " + shell_quote(target) +
                " >/dev/null 2>&1");
        require_run("git -C " + shell_quote(target) +
                " config user.email prompt-test@example.invalid");
        require_run("git -C " + shell_quote(target) + " config user.name Prompt Test");
}

void test_git_sync_states() {
        fs::path root = make_temp_dir("git_sync");
        fs::path remote = root / "origin.git";
        require_run("git init --bare " + shell_quote(remote) + " >/dev/null 2>&1");

        fs::path seed = root / "seed";
        clone_and_config(remote, seed);
        require_run("git -C " + shell_quote(seed) + " checkout -b main >/dev/null 2>&1");
        write_file(seed / "file.txt", "base\n");
        commit_all(seed, "initial");
        require_run("git -C " + shell_quote(seed) +
                " push -u origin main >/dev/null 2>&1");
        require_run("git -C " + shell_quote(remote) +
                " symbolic-ref HEAD refs/heads/main");

        fs::path behind = root / "behind";
        clone_and_config(remote, behind);
        fs::path writer = root / "writer";
        clone_and_config(remote, writer);
        write_file(writer / "remote.txt", "remote\n");
        commit_all(writer, "remote");
        require_run("git -C " + shell_quote(writer) + " push >/dev/null 2>&1");
        require_run("git -C " + shell_quote(behind) + " fetch >/dev/null 2>&1");
        {
                ScopedCwd cwd(behind);
                check(contains(get_git_pill().zsh_code, "⇣1"), "behind token");
        }

        fs::path ahead = root / "ahead";
        clone_and_config(remote, ahead);
        write_file(ahead / "ahead.txt", "ahead\n");
        commit_all(ahead, "ahead");
        {
                ScopedCwd cwd(ahead);
                check(contains(get_git_pill().zsh_code, "⇡1"), "ahead token");
        }

        fs::path diverged = root / "diverged";
        clone_and_config(remote, diverged);
        write_file(diverged / "local.txt", "local\n");
        commit_all(diverged, "local");
        fs::path writer2 = root / "writer2";
        clone_and_config(remote, writer2);
        write_file(writer2 / "remote2.txt", "remote2\n");
        commit_all(writer2, "remote2");
        require_run("git -C " + shell_quote(writer2) + " push >/dev/null 2>&1");
        require_run("git -C " + shell_quote(diverged) + " fetch >/dev/null 2>&1");
        {
                ScopedCwd cwd(diverged);
                check(contains(get_git_pill().zsh_code, "⇕1/1"), "diverged token");
        }
}

void test_git_detached_and_conflict() {
        fs::path detached = make_temp_dir("git_detached");
        init_repo(detached);
        write_file(detached / "file.txt", "base\n");
        commit_all(detached, "initial");
        require_run("git -C " + shell_quote(detached) +
                " checkout --detach HEAD >/dev/null 2>&1");
        {
                ScopedCwd cwd(detached);
                check(contains(get_git_pill().zsh_code, "detached@"), "detached head");
        }

        fs::path conflict = make_temp_dir("git_conflict");
        init_repo(conflict);
        write_file(conflict / "conflict.txt", "base\n");
        commit_all(conflict, "initial");
        require_run("git -C " + shell_quote(conflict) +
                " checkout -b feature >/dev/null 2>&1");
        write_file(conflict / "conflict.txt", "feature\n");
        commit_all(conflict, "feature");
        require_run("git -C " + shell_quote(conflict) +
                " checkout main >/dev/null 2>&1");
        write_file(conflict / "conflict.txt", "main\n");
        commit_all(conflict, "main");
        std::system(("git -C " + shell_quote(conflict) +
                " merge feature >/dev/null 2>&1").c_str());
        {
                ScopedCwd cwd(conflict);
                RenderedPill pill = get_git_pill();
                check(contains(pill.zsh_code, "MERGE"), "merge operation");
                check(contains(pill.zsh_code, "!1"), "conflict token");
        }
}

} // namespace

int main() {
        try {
                test_visible_width();
                test_directory_paths();
                test_language_detection();
                test_git_non_git_and_clean();
                test_git_dirty_changes();
                test_git_renamed();
                test_git_sync_states();
                test_git_detached_and_conflict();
        } catch (const std::exception &error) {
                std::cerr << "test failed: " << error.what() << '\n';
                return 1;
        }

        std::cout << "all tests passed\n";
        return 0;
}
