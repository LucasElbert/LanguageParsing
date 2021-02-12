#include "utils.h"

bool AllTrue(vector<bool>& values) {
  for (bool b : values) {
    if (!b) return false;
  }
  return true;
}

bool AnyTrue(vector<bool>& values) {
  for (bool b : values) {
    if (b) return true;
  }
  return false;
}

tuple<int, char, string> ReadUntil(string s, int start, string stop_chars) {
  int i = start;
  while (i < s.length() && stop_chars.find(s[i]) == string::npos) {
    i++;
  }
  bool found = i < s.length();

  if (found) {
    char c = s[i];
    string text = s.substr(start, i - start);
    return std::make_tuple(i, c, text);
  } else {
    return std::make_tuple(string::npos, NULL, "");
  }
}
