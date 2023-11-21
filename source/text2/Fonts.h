#include "./Font.h"
#include "../EsUuid.h"

#include <cstdint>
#include <vector>

namespace Font2s {
    class ID {
    public:
        ID();
        bool operator==(const ID &other) const {
            return fontID == other.fontID;
        }
        bool operator>(const ID &other) const {
            return fontID > other.fontID;
        }
        bool operator<(const ID &other) const {
            return fontID < other.fontID;
        }

    private:
        uint_fast32_t fontID;
    };

    enum class Style {
        REGULAR,
        BOLD,
        ITALIC,
        MONOSPACE,
    };

    ID Load(const std::vector<char> &data, uint32_t size);
    Font2 &Get(ID id);
    Font2 &MatchGlyph(uint32_t glyphID);
}
