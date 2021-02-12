#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <tuple>
#include <string>

using std::tuple;
using std::vector;
using std::string;

// Tests whether all entries of values are true
bool AllTrue(vector<bool>& values);

// Tests whether at least one of value's entries is true
bool AnyTrue(vector<bool>& values);

// Reads the string s starting from index start until occurence of any character
// in stop_chars
// Returns the first found stop_char its index and the substring from start to
// the found stop_char.
tuple<int, char, string> ReadUntil(string s, int start, string stop_chars);

#endif