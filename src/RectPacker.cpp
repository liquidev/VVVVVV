#include "RectPacker.h"

#include "Vlogging.h"
#include <utility>

RectPacker::RectPacker(int w, int h)
: _width(w)
, _height(h)
, _nodes{{{0}}}
, _n_nodes(1)
{
    _nodes[0] = {
        { 0, 0, w, h }, // area
        false,          // occupied
    };
}

size_t RectPacker::push(RectPackerNode &&node)
{
    if (_n_nodes >= MaxNodes - 1)
        return -1;

    size_t index = _n_nodes;
    _nodes[_n_nodes] = node;
    _n_nodes++;
    return index;
}

int RectPacker::findFree(int w, int h)
{
    vlog_debug("(RectPacker) finding free texture %i x %i", w, h);
    for (size_t i = 0; i < _n_nodes; ++i) {
        const auto &node = _nodes[i];
        vlog_debug("(RectPacker) checking node %u (%i x %i occupied=%i)", i, node.area.w, node.area.h, node.occupied);
        if (!node.occupied && w <= node.area.w && h <= node.area.h) {
            return i;
        }
    }
    return -1;
}

bool RectPacker::subdivide(size_t i, int w, int h, int &out_x, int &out_y)
{
    auto &node = _nodes[i];
    const auto narea = node.area;

    if (w == narea.w && h == narea.h) {
        // First edge case: width and height matches - we just mark the node as occupied
    } else if (h == narea.h) {
        // Second edge case: height matches - we do a horizontal split only
        RectPackerNode split = {
            { int(narea.x + w), narea.y, int(narea.w - w), narea.h },
            false,
        };
        if (!push(std::move(split)))
            return false;
    } else if (w == narea.w) {
        // Third edge case: width matches - we do a vertical split only
        RectPackerNode split = {
            { narea.x, int(narea.y + h), narea.w, int(narea.h - h) },
            false,
        };
        if (!push(std::move(split)))
            return false;
    } else {
        // Normal case: neither width nor height matches and we have to do two splits.
        RectPackerNode rightSplit = {
            { int(narea.x + w), narea.y, int(narea.w - w), int(h) },
            false,
        };
        RectPackerNode bottomSplit = {
            { narea.x, int(narea.y + h), narea.w, int(narea.h - h) },
            false,
        };
        if (!push(std::move(rightSplit)))
            return false;
        if (!push(std::move(bottomSplit)))
            return false;
    }

    node.area = { narea.x, narea.y, int(w), int(h) };
    node.occupied = true;
    return true;
}

bool RectPacker::pack(SDL_Rect &inout)
{
    const auto free = findFree(inout.w, inout.h);
    if (free == -1) {
        vlog_debug("(RectPacker) no unoccupied rectangles found");
        return false;
    }

    return subdivide(free, inout.w, inout.h, inout.x, inout.y);
}
