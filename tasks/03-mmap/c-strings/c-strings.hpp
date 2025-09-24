#pragma once

#include <cstddef>

// Non-allocating functions
size_t StrLen(const char* str);
size_t StrNLen(const char* str, size_t limit);
int StrCmp(const char* lhs, const char* rhs);
int StrNCmp(const char* lhs, const char* rhs, size_t limit);
char* StrCat(char* s1, const char* s2);
const char* StrStr(const char* haystack, const char* needle);

// Allocating functions
char* StrDup(const char* str);
char* StrNDup(const char* str, size_t limit);
char* AStrCat(const char* s1, const char* s2);
void Deallocate(char*);
