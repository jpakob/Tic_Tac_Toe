#include <unity.h>
#include "game.h"

void setUp() {}
void tearDown() {}

void test_gewinn_reihe() {
    uint8_t b[9] = {1,1,1, 0,0,0, 0,0,0};
    TEST_ASSERT_EQUAL_UINT8(1, winner(b));
}

void test_kein_gewinner() {
    uint8_t b[9] = {1,2,1, 0,0,0, 0,0,0};
    TEST_ASSERT_EQUAL_UINT8(0, winner(b));
}

void test_index_leer() {
    uint8_t b[9] = {0};
    TEST_ASSERT_EQUAL_UINT16(0, encodeState(b));
}

void test_alle_gewinnlinien() {
    const uint8_t L[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};
    for (int i = 0; i < 8; i++) {
        for (uint8_t p = 1; p <= 2; p++) {
            uint8_t b[9] = {0};
            b[L[i][0]] = b[L[i][1]] = b[L[i][2]] = p;
            TEST_ASSERT_EQUAL_UINT8(p, winner(b));
        }
    }
}

void test_remis() {
    uint8_t b[9] = {1,2,1, 1,2,2, 2,1,1};
    TEST_ASSERT_TRUE(isFull(b));
    TEST_ASSERT_EQUAL_UINT8(0, winner(b));
}

void test_index_kodierung() {
    uint8_t a[9] = {1,0,0, 0,0,0, 0,0,0};
    uint8_t c[9] = {0,1,0, 0,0,0, 0,0,0};
    uint8_t d[9] = {2,2,2, 2,2,2, 2,2,2};
    TEST_ASSERT_EQUAL_UINT16(1, encodeState(a));
    TEST_ASSERT_EQUAL_UINT16(3, encodeState(c));
    TEST_ASSERT_EQUAL_UINT16(19682, encodeState(d));   // höchster Index
}

void test_legal_moves() {
    uint8_t b[9] = {1,0,2, 0,0,0, 0,0,0};
    uint8_t m[9];
    TEST_ASSERT_EQUAL_UINT8(7, legalMoves(b, m));
    TEST_ASSERT_EQUAL_UINT8(1, m[0]);   // erstes freies Feld ist Index 1
}

void test_winline_leer() {
    uint8_t b[9] = {0}, line[3];
    TEST_ASSERT_FALSE(winLine(b, line));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_gewinn_reihe);
    RUN_TEST(test_kein_gewinner);
    RUN_TEST(test_index_leer);
    RUN_TEST(test_alle_gewinnlinien);
    RUN_TEST(test_remis);
    RUN_TEST(test_index_kodierung);
    RUN_TEST(test_legal_moves);
    RUN_TEST(test_winline_leer);
    return UNITY_END();
}