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


vector<string> split(string str, char sep) {
  vector<string> tokens;
  int start = 0;
  int end = 0;
  do {
    end = str.find(sep, start);
    if (end == string::npos) {
      end = str.size();
    }
    if (end > start) {
      // end == start could happen with a sequence of separating chars
      tokens.push_back(str.substr(start, end-start));
    }
    start = end+1;
  } while (end != str.size());
  
  return tokens;
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
