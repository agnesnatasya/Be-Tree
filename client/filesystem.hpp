// Defines the Key to use with the betree as the one used in Betrfs 
#include <iostream>
#include <sstream>
#include <vector>

class FKey {
public:
  FKey(void) :
    key()
  {}

  FKey(const std::string &k) :
    key(k)
  {
    tokens = split_path();
  }

  std::vector<std::string> split_path() {
    std::vector<std::string> result;
    std::stringstream ss (key);
    std::string item;

    while (getline (ss, item, '/')) {
        result.push_back (item);
    }

    return result;
  }

  bool operator<(const FKey &k) const {
    if (key == k.key) return false;
    if (tokens.size() < k.tokens.size()) return true;
    if (tokens.size() > k.tokens.size()) return false;

    for (uint8_t i = 0; i < tokens.size(); i++) {
      if (tokens[i] > k.tokens[i]) return false;
      if (tokens[i] < k.tokens[i]) return true;
    }

    return true;
  }

  bool operator>(const FKey &k) const {
    if (key == k.key) return false;
    if (tokens.size() > k.tokens.size()) return true;
    if (tokens.size() < k.tokens.size()) return false;

    for (uint8_t i = 0; i < tokens.size(); i++) {
      if (tokens[i] < k.tokens[i]) return false;
      if (tokens[i] > k.tokens[i]) return true;
    }

    return true;
  }

  bool operator==(const FKey &k) const {
    return key == k.key;
  }

  bool operator!=(const FKey &k) const {
    return key != k.key;
  }

  void _serialize(std::iostream &fs, serialization_context &context) const {
    fs << key << " ";
    serialize(fs, context, key);
  }

  void _deserialize(std::iostream &fs, serialization_context &context) {
    fs >> key;
    deserialize(fs, context, key);
  }

  std::string key;
  std::vector<std::string> tokens;
};

