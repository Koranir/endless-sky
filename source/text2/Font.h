#include <cstdint>
#include <vector>

class Font2 {
public:
    Font2(const std::vector<char> &data, uint32_t size) {};
    bool Has(const uint32_t glyphID) {};
};
