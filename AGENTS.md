# Repository Guidelines

## Project Structure & Module Organization

This repository contains a small C++ prompt-rendering program for Zsh.

- `main.cpp` is the entry point. It parses optional arguments for exit code, command duration, and terminal width, then assembles the two-line prompt.
- `modules.cpp` contains prompt modules for language detection, directory display, Git status, and shell command execution.
- `utils.h` defines shared color constants, the `RenderedPill` struct, and shared function declarations.

There is currently no `src/`, `tests/`, or asset directory. Keep new files at the root only while the project stays this small; introduce directories once there are multiple related files.

## Build, Test, and Development Commands

- `g++ -std=c++17 -Wall -Wextra -pedantic main.cpp modules.cpp -o prompt_dev` builds the executable. C++17 is required for `std::filesystem`.
- `./prompt_dev 0 120ms 100` runs a sample successful prompt render.
- `./prompt_dev 1 0ms 80` runs a sample failing prompt render.
- `git status --porcelain -b` is useful when checking Git pill behavior, because `modules.cpp` parses this output.

No Makefile or package manifest is checked in yet. If you add one, keep these direct commands working or document the replacement here.

## Coding Style & Naming Conventions

Use C++17 and standard library facilities already present in the codebase. Match the current style: braces on the same line for functions and control blocks, short helper structs, snake_case function names, and descriptive local variables. Indent continuation lines clearly and keep prompt width calculations close to the strings they describe.

Prefer `const` values for shared prompt colors in `utils.h`. Keep Zsh escape sequences and visible-width accounting synchronized whenever modifying rendered text.

## Testing Guidelines

There is no automated test suite yet. For every prompt change, build with warnings enabled and manually run examples with clean, dirty, and non-Git directories. Verify that `visible_width` changes match the rendered content so right alignment remains stable.

If tests are added, use focused cases for directory truncation, language detection priority, Git status parsing, and argument defaults.

## Commit & Pull Request Guidelines

The current Git history uses short messages such as `update`; keep commits concise, imperative, and scoped, for example `adjust git status icons` or `add prompt width tests`.

Pull requests should include a brief description, the build command used, manual test cases run, and screenshots or terminal captures for visual prompt changes. Link related issues when applicable.
