#include "VPSPFile.h"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "Exit.h"
#include "Vlogging.h"

namespace g {
    FILE *mount = nullptr;

    VPSP::Entry entries[256] = {{{0}}};
    unsigned n_entries;

    struct {
        bool open;
        uint8_t data[VPSP_MAX_SIZE];
    } open_files[VPSP_MAX_FILES];
}

static void parse()
{
    char discard[3];

    char magic[5] = {0};
    uint8_t n_entries;
    unsigned file_offset = 0;

    file_offset += sizeof(magic) - 1; // Magic bytes
    file_offset += sizeof(uint8_t) * 4; // Entry count (only first byte is used)

    // Check magic.
    fread(magic, 4, 1, g::mount);
    if (strcmp(magic, "VPSP") != 0) {
        vlog_error("(VPSP) Invalid header\n");
        VVV_exit(1);
    }

    // Read entry count.
    fread(&n_entries, 1, 1, g::mount);
    g::n_entries = n_entries;
    // Discard 3 trailing bytes.
    fread(&discard, 1, 3, g::mount);

    // Read entries.
    for (unsigned i = 0; i < n_entries; ++i) {
        VPSP::Entry *e = &g::entries[i];
        memset(e->path, 0, VPSP::Entry::PathLength);

        // Read path length.
        fread(&e->path_length, 1, 4, g::mount);
        if (e->path_length >= VPSP::Entry::PathLength - 1) {
            vlog_error("(VPSP) Path length exceeded %u characters\n", VPSP::Entry::PathLength);
            VVV_exit(1);
        }
        file_offset += sizeof(uint32_t);

        // Read the path.
        fread(e->path, e->path_length, 1, g::mount);
        file_offset += e->path_length;

        // Read the file size.
        fread(&e->file_length, sizeof(uint32_t), 1, g::mount);
        file_offset += sizeof(uint32_t);
    }

    // Compute file offsets for all entries.
    for (unsigned i = 0; i < n_entries; ++i) {
        VPSP::Entry *e = &g::entries[i];
        e->file_offset = file_offset;
        file_offset += e->file_length;
    }
}

static uint8_t *openFile()
{
    for (int i = 0; i < VPSP_MAX_FILES; ++i) {
        if (!g::open_files[i].open) {
            return g::open_files[i].data;
        }
    }
    vlog_error("(VPSP) Maximum limit of open files reached\n");
    VVV_exit(1);
    return nullptr;
}

void VPSP::mount(const char *path)
{
    if (g::mount != nullptr) {
        fclose(g::mount);
    }
    g::mount = fopen(path, "rb");
    if (g::mount == nullptr) {
        vlog_error("(VPSP) Could not open %s: %s\n", path, strerror(errno));
        VVV_exit(1);
    }
    parse();
}

MemFile VPSP::openMemFile(const char *path)
{
}
