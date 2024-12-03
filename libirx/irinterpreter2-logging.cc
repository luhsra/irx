#include <ostream>

class NullStream : public std::ostream {
// https://stackoverflow.com/a/59673391
public:
  NullStream() : std::ostream(nullptr) {}
  NullStream(const NullStream &) : std::ostream(nullptr) {}
};

template <class T>
const NullStream &operator<<(NullStream &&os, const T &value) { 
  return os;
}

auto null = NullStream();

std::ostream& get_nulls() {
    return null;
}
