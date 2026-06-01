#include "avail/input.h"

#include <cstring>
#include <gtest/gtest.h>

namespace {

avail::KeyEvent parse(const char* s) {
    return avail::parse_key_sequence(s, std::strlen(s));
}

TEST(input_parse_test, lf_is_enter) {
    const auto ev = parse("\n");
    EXPECT_EQ(ev.key, avail::Key::Enter);
}

TEST(input_parse_test, cr_is_enter) {
    const auto ev = parse("\r");
    EXPECT_EQ(ev.key, avail::Key::Enter);
}

TEST(input_parse_test, bare_esc_is_esc) {
    const char esc[] = {0x1B};
    const auto ev = avail::parse_key_sequence(esc, 1);
    EXPECT_EQ(ev.key, avail::Key::Esc);
}

TEST(input_parse_test, csi_a_is_up) {
    const char seq[] = {0x1B, '[', 'A'};
    const auto ev = avail::parse_key_sequence(seq, 3);
    EXPECT_EQ(ev.key, avail::Key::Up);
}

TEST(input_parse_test, csi_b_is_down) {
    const char seq[] = {0x1B, '[', 'B'};
    const auto ev = avail::parse_key_sequence(seq, 3);
    EXPECT_EQ(ev.key, avail::Key::Down);
}

TEST(input_parse_test, csi_c_is_right) {
    const char seq[] = {0x1B, '[', 'C'};
    const auto ev = avail::parse_key_sequence(seq, 3);
    EXPECT_EQ(ev.key, avail::Key::Right);
}

TEST(input_parse_test, csi_d_is_left) {
    const char seq[] = {0x1B, '[', 'D'};
    const auto ev = avail::parse_key_sequence(seq, 3);
    EXPECT_EQ(ev.key, avail::Key::Left);
}

TEST(input_parse_test, csi_unknown_is_unknown) {
    const char seq[] = {0x1B, '[', 'Z'};
    const auto ev = avail::parse_key_sequence(seq, 3);
    EXPECT_EQ(ev.key, avail::Key::Unknown);
}

TEST(input_parse_test, plain_char_is_char) {
    const auto ev = parse("q");
    EXPECT_EQ(ev.key, avail::Key::Char);
    EXPECT_EQ(ev.ch, 'q');
}

TEST(input_parse_test, char_b_is_char) {
    const auto ev = parse("b");
    EXPECT_EQ(ev.key, avail::Key::Char);
    EXPECT_EQ(ev.ch, 'b');
}

TEST(input_parse_test, empty_sequence_is_unknown) {
    const auto ev = avail::parse_key_sequence(nullptr, 0);
    EXPECT_EQ(ev.key, avail::Key::Unknown);
}

TEST(input_parse_test, esc_followed_by_non_bracket_is_esc) {
    const char seq[] = {0x1B, 'x', '\0'};
    const auto ev = avail::parse_key_sequence(seq, 3);
    EXPECT_EQ(ev.key, avail::Key::Esc);
}

TEST(input_parse_test, esc_short_sequence_is_esc) {
    const char seq[] = {0x1B, '['};
    const auto ev = avail::parse_key_sequence(seq, 2);
    EXPECT_EQ(ev.key, avail::Key::Esc);
}

TEST(input_parse_test, ch_zero_when_key_is_arrow) {
    const char seq[] = {0x1B, '[', 'A'};
    const auto ev = avail::parse_key_sequence(seq, 3);
    EXPECT_EQ(ev.ch, 0);
}

}  // namespace
