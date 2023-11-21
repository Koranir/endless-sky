#include "./Font.h"
#include "./Format.h"

#include <string>
#include <utility>
#include <vector>

class Text2 {
public:
    void draw(const Font2 &font, const std::pair<Point, Point> &bounds, const Format2 &formatting);

private:
    std::string contents;

};
