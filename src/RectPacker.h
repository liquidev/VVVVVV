// A rect packer, used for efficiently allocating framebuffers.

#ifndef RECT_PACKER_H
#define RECT_PACKER_H

#include <SDL2/SDL.h>

struct RectPackerNode
{
    SDL_Rect area;
    bool occupied;
};

class RectPacker
{
    constexpr static int MaxNodes = 27;

    unsigned _width, _height;
    RectPackerNode _nodes[MaxNodes];
    size_t _n_nodes;

    // Adds a new node at the end of the list. Returns -1 if all slots are taken.
    size_t push(RectPackerNode &&node);
    // Finds an unoccupied node that can fit a rect of the given size.
    // Returns -1 if no node could be found.
    int findFree(int w, int h);
    // Subdivides a node with the given index and writes the final coordinates to out_x, out_y.
    // out_x and out_y are integers because SDL_Rects store integers.
    bool subdivide(size_t i, int w, int h, int &out_x, int &out_y);

public:
    RectPacker(int w, int h);

    // Packs a rectangle onto the surface. The size is read from the provided rectangle and the
    // coordinates are written back to it if free space was found.
    //
    // Returns false if no more space is available, otherwise returns true.
    bool pack(SDL_Rect &inout);
};

#endif
