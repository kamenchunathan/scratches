#include <cstdlib>
#include <cstring>
#include <print>
#include <vector>

#include "color.hpp"
#include "term/ansi.hpp"
#include "term/diff.hpp"

int main(int argv, char* argc[]) {
    if (argv <= 2) {
        std::println("Insufficient arguments");
        std::exit(1);
    }
    auto a = std::vector(argc[1], argc[1] + std::strlen(argc[1]));
    auto b = std::vector(argc[2], argc[2] + std::strlen(argc[2]));

    MyersDiff<char> diff(a, b);

    std::vector<Snake> trace;
    diff.build_trace(Box(0, 0, a.size(), b.size()), trace);

    for (auto snake: trace) {
        std::println("{}", snake);
    }

    auto edits = build_edit_script(a, b, trace);
    for (auto edit: edits.edits()) {
        switch (edit.type) {
            case EditType::Keep:
                std::print("{}", a[edit.src_pos]);
                break;
            case EditType::Delete:
                ansi::scoped(
                    std::cout,
                    [&](std::ostream& os) { std::print(os, "-{}", a[edit.src_pos]); },
                    ansi::fg::scoped_color(core::ColorRGB8::MAGENTA)
                );
                break;
            case EditType::Insert:
                ansi::scoped(
                    std::cout,
                    [&](std::ostream& os) { std::print(os, "+{}", b[edit.tgt_pos]); },
                    ansi::fg::scoped_color(core::ColorRGB8::GREEN)
                );
                break;
        }
    }
    std::println("");
}
