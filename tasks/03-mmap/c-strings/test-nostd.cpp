#include "c-strings.hpp"

#include <assert.hpp>
#include <syscalls.hpp>

void TestStrLen() {
    ASSERT(StrLen("") == 0);
    ASSERT(StrLen("aba") == 3);
    ASSERT(StrNLen("", 10) == 0);
    ASSERT(StrNLen("", 0) == 0);
    ASSERT(StrNLen("aba", 10) == 3);
    ASSERT(StrNLen("aba", 2) == 2);
}

void TestStrCmp() {
    ASSERT(StrCmp("", "aba") < 0);
    ASSERT(StrCmp("", "") == 0);
    ASSERT(StrCmp("aba", "abac") < 0);
    ASSERT(StrCmp("aba", "aba") == 0);
    ASSERT(StrCmp("\xff", "\xcd") > 0);
    ASSERT(StrCmp("\xaa", "\xbb") < 0);

    auto a = "asdf";
    auto b = "asdfghjk";
    for (size_t i = 0; i < 10; ++i) {
        if (i <= 4) {
            ASSERT(StrNCmp(a, b, i) == 0);
        } else {
            ASSERT(StrNCmp(a, b, i) < 0);
            ASSERT(StrNCmp(b, a, i) > 0);
        }
    }
}

void TestStrCat() {
    char buf[10] = "";
    char* s;

    s = StrCat(buf, "aba");
    ASSERT(s == buf);
    ASSERT(StrCmp(buf, "aba") == 0);

    s = StrCat(buf, "caba");
    ASSERT(s == buf);
    ASSERT(StrCmp(buf, "abacaba") == 0);

    s = StrCat(buf, "da");
    ASSERT(s == buf);
    ASSERT(StrCmp(buf, "abacabada") == 0);
}

void TestStrStr() {
    const char* haystack;

    haystack = "";
    ASSERT(StrStr(haystack, "") == haystack);
    ASSERT(StrStr(haystack, "daba") == nullptr);

    haystack = "abacaba";
    ASSERT(StrStr(haystack, "") == haystack);
    ASSERT(StrStr(haystack, "cab") == haystack + 3);
    ASSERT(StrStr(haystack, haystack) == haystack);
    ASSERT(StrStr(haystack, "daba") == nullptr);
    ASSERT(StrStr(haystack, "aaaaaaaaaaa") == nullptr);
}

int Main(int, char**, char**) {
    RUN_TEST(TestStrLen);
    RUN_TEST(TestStrCmp);
    RUN_TEST(TestStrCat);
    RUN_TEST(TestStrStr);

    return 0;
}
