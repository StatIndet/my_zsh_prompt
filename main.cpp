#include "utils.h"
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
        int exit_code = (argc > 1) ? std::stoi(argv[1]) : 0;
        std::string duration = (argc > 2) ? argv[2] : "";
        int term_width = (argc > 3) ? std::stoi(argv[3]) : 80;

        std::string time_color = (exit_code == 0) ? C_YELLOW : C_RED;

        RenderedPill wave_link = render_text(C_GRAY, " 󰜥 ");

        // ====== 构建第一行左侧 ======
        RenderedPill prompt_start = render_text(C_GRAY, "╭─ ");
        std::string left_prompt = "\n" + prompt_start.zsh_code;
        int left_width = prompt_start.visible_width;

        if (!duration.empty() && duration != "0ms") {
                RenderedPill duration_pill =
                        render_pill(C_YELLOW, "󰔚 " + duration + " ");
                left_prompt += duration_pill.zsh_code + wave_link.zsh_code;
                left_width += duration_pill.visible_width + wave_link.visible_width;
        }

        RenderedPill dir_pill = get_directory_pill();
        left_prompt += dir_pill.zsh_code;
        left_width += dir_pill.visible_width;

        RenderedPill git_pill = get_git_pill();
        if (git_pill.visible_width > 0) {
                left_prompt += wave_link.zsh_code + git_pill.zsh_code;
                left_width += wave_link.visible_width + git_pill.visible_width;
        }

        // ====== 构建第一行右侧 ======
        std::string right_prompt = "";
        int right_width = 0;

        RenderedPill lang_pill = get_lang_env();
        if (lang_pill.visible_width > 0) {
                right_prompt += lang_pill.zsh_code + wave_link.zsh_code;
                right_width += lang_pill.visible_width + wave_link.visible_width;
        }

        RenderedPill time_pill =
                render_pill(time_color, "  %D{%H:%M:%S} ", "  00:00:00 ");

        right_prompt += time_pill.zsh_code;
        right_width += time_pill.visible_width;

        // ====== 对齐 ======
        int spaces_needed = term_width - left_width - right_width;
        if (spaces_needed < 0)
                spaces_needed = 0;
        std::string fill_spaces(spaces_needed, ' ');

        // ====== 第二行光标输入层 ======
        std::string second_line = "\n" + render_text(C_GRAY, "╰─ ").zsh_code +
                render_text(C_GREEN, " ").zsh_code;

        std::cout << left_prompt << fill_spaces << right_prompt << second_line;

        return 0;
}
