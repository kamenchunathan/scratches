#include "gtest/gtest.h"

#include "term/input_parser.hpp"

using namespace core::input;

// Helper to check for a specific KeyEvent
static void ExpectKeyEvent(
    const Event& event,
    KeyCode code,
    bool shift = false,
    bool ctrl = false,
    bool alt = false
) {
    ASSERT_TRUE(std::holds_alternative<KeyEvent>(event));
    auto key_event = std::get<KeyEvent>(event);
    EXPECT_EQ(key_event.code, code);
    EXPECT_EQ(key_event.action, KeyEvent::Action::Press);
    EXPECT_EQ(key_event.shift, shift);
    EXPECT_EQ(key_event.ctrl, ctrl);
    EXPECT_EQ(key_event.alt, alt);
}

// Helper to check for a specific MouseEvent
static void ExpectMouseEvent(
    const Event& event,
    MouseEvent::Action action,
    uint32_t col,
    uint32_t row,
    bool shift = false,
    bool ctrl = false,
    bool alt = false
) {
    ASSERT_TRUE(std::holds_alternative<MouseEvent>(event));
    auto mouse_event = std::get<MouseEvent>(event);
    EXPECT_EQ(mouse_event.action, action);
    EXPECT_EQ(mouse_event.col, col);
    EXPECT_EQ(mouse_event.row, row);
    EXPECT_EQ(mouse_event.shift, shift);
    EXPECT_EQ(mouse_event.ctrl, ctrl);
    EXPECT_EQ(mouse_event.alt, alt);
}

TEST(AnsiInputParserTest, ParsesSingleCharacters) {
    InputParser parser("abc");
    auto events = parser.parse();
    ASSERT_EQ(events.size(), 3);
    ExpectKeyEvent(events[0], KeyCode::A);
    ExpectKeyEvent(events[1], KeyCode::B);
    ExpectKeyEvent(events[2], KeyCode::C);
}

TEST(AnsiInputParserTest, ParsesEmptyInput) {
    InputParser parser("");
    auto events = parser.parse();
    EXPECT_TRUE(events.empty());
}

TEST(AnsiInputParserTest, SkipsUnrecognizedEscapeSequence) {
    // \x1b[2J is "clear screen"
    InputParser parser("a\x1b[2Jb");
    auto events = parser.parse();
    ASSERT_EQ(events.size(), 2);
    ExpectKeyEvent(events[0], KeyCode::A);
    ExpectKeyEvent(events[1], KeyCode::B);
}

TEST(AnsiInputParserTest, HandlesIncompleteEscapeSequence) {
    InputParser parser("a\x1b[<0;1"); // Incomplete
    auto events = parser.parse();
    ASSERT_EQ(events.size(), 1);
    ExpectKeyEvent(events[0], KeyCode::A);
}

TEST(AnsiInputParserTest, ParsesSgrMousePress) {
    InputParser parser("\x1b[<0;42;21M");
    auto events = parser.parse();
    ASSERT_EQ(events.size(), 1);
    ExpectMouseEvent(events[0], MouseEvent::Action::Press, 42, 21);
}

TEST(AnsiInputParserTest, ParsesSgrMouseRelease) {
    InputParser parser("\x1b[<2;10;5m");
    auto events = parser.parse();
    ASSERT_EQ(events.size(), 1);
    ExpectMouseEvent(events[0], MouseEvent::Action::Release, 10, 5);
}

TEST(AnsiInputParserTest, ParsesSgrMousePressWithModifiers) {
    // Cb = 0 (Left) + 4 (Shift) + 16 (Ctrl) = 20
    InputParser parser("\x1b[<20;33;8M");
    auto events = parser.parse();
    ASSERT_EQ(events.size(), 1);
    ExpectMouseEvent(events[0], MouseEvent::Action::Press, 33, 8, true, true, false);
}

TEST(AnsiInputParserTest, ParsesSgrMouseMoveWithModifiers) {
    // Cb = 32 (Move) + 0 (Left) + 8 (Alt) = 40
    InputParser parser("\x1b[<40;15;12M");
    auto events = parser.parse();
    ASSERT_EQ(events.size(), 1);
    ExpectMouseEvent(events[0], MouseEvent::Action::Move, 15, 12, false, false, true);
}

TEST(AnsiInputParserTest, ParsesArrowKeys) {
    InputParser parser("\x1b[A\x1b[B\x1b[C\x1b[D");
    auto events = parser.parse();
    ASSERT_EQ(events.size(), 4);
    ExpectKeyEvent(events[0], KeyCode::Up);
    ExpectKeyEvent(events[1], KeyCode::Down);
    ExpectKeyEvent(events[2], KeyCode::Right);
    ExpectKeyEvent(events[3], KeyCode::Left);
}

TEST(AnsiInputParserTest, ParsesModifiedArrowKeys) {
    // Ctrl+Up: ESC [ 1 ; 5 A
    InputParser parser("\x1b[1;5A");
    auto events = parser.parse();
    ASSERT_EQ(events.size(), 1);
    ExpectKeyEvent(events[0], KeyCode::Up, false, true, false);
}

TEST(AnsiInputParserTest, ParsesShiftArrowKey) {
    // Shift+Down: ESC [ 1 ; 2 B
    InputParser parser("\x1b[1;2B");
    auto events = parser.parse();
    ASSERT_EQ(events.size(), 1);
    ExpectKeyEvent(events[0], KeyCode::Down, true, false, false);
}

TEST(AnsiInputParserTest, ParsesMixedContent) {
    InputParser parser("x\x1b[<0;1;2My\x1b[1;5Az");
    auto events = parser.parse();
    ASSERT_EQ(events.size(), 4);
    ExpectKeyEvent(events[0], KeyCode::X);
    ExpectMouseEvent(events[1], MouseEvent::Action::Press, 1, 2);
    ExpectKeyEvent(events[2], KeyCode::Y);
    ExpectKeyEvent(events[3], KeyCode::Up, false, true, false);
}

TEST(AnsiInputParserTest, HandlesNonCsiEscapeCode) {
    // ESC P is DCS, which we don't handle. Should be skipped.
    InputParser parser("a\x1bPb");
    auto events = parser.parse();
    ASSERT_EQ(events.size(), 2);
    ExpectKeyEvent(events[0], KeyCode::A);
    ExpectKeyEvent(events[1], KeyCode::B);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
