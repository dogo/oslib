#include "oslib.h"

OSL_MAP *oslCreateMap(OSL_IMAGE *img, void *map_data, int tileX, int tileY, int mapSizeX, int mapSizeY, int map_format) {
    if (!img || !map_data) {
        oslFatalError("Invalid input: img or map_data is NULL");
        return NULL;
    }

    if (tileX <= 0 || tileY <= 0 || mapSizeX <= 0 || mapSizeY <= 0) {
        oslFatalError("Invalid input: tile or map dimensions must be positive");
        return NULL;
    }

    OSL_MAP *m = (OSL_MAP*)calloc(1, sizeof(OSL_MAP));
    if (!m) {
        return NULL;
    }

    if (map_format != OSL_MF_U16 && map_format != OSL_MF_U16_GBA) {
        oslFatalError("Invalid map format");
        free(m);
        return NULL;
    }

    m->format = map_format;
    m->flags = OSL_MF_TILE1_TRANSPARENT;

    if (map_format == OSL_MF_U16_GBA) {
        m->addit1 = 10;
    }

    m->img = img;
    m->map = map_data;
    m->tileX = tileX;
    m->tileY = tileY;
    m->mapSizeX = mapSizeX;
    m->mapSizeY = mapSizeY;
    m->scrollX = m->scrollY = 0;
    m->drawSizeX = -1;
    m->drawSizeY = -1;

    return m;
}

void oslDrawMapSimple(OSL_MAP *m) {
    oslDrawMap(m);
}

void oslDrawMap(OSL_MAP *m) {
    if (!m || !m->img || !m->map) return;

    int x, y, v, sX, sY, mX, mY, dX, bY, dsX, dsY, xTile, yTile;
    u32 tilesPerLine = m->img->sizeX / m->tileX;
    u32 firstTileOpaque = !(m->flags & OSL_MF_TILE1_TRANSPARENT);
    u16 *map = (u16*)m->map;
    OSL_FAST_VERTEX *vertices;
    int nbVertices;
    int tilesPerLineOpt = 0;

    oslSetTexture(m->img);

    if (m->drawSizeX < 0 || m->drawSizeY < 0) {
        dsX = osl_curBuf->sizeX / m->tileX + 1;
        if (osl_curBuf->sizeX % m->tileX) dsX++;
        dsY = osl_curBuf->sizeY / m->tileY + 1;
        if (osl_curBuf->sizeY % m->tileY) dsY++;
    } else {
        dsX = m->drawSizeX;
        dsY = m->drawSizeY;
    }

    // Optimize tilesPerLineOpt
    for (int i = 1; i <= 8; i++) {
        if (tilesPerLine >= (1 << i)) {
            tilesPerLineOpt = i;
        }
    }

    sX = m->scrollX % m->tileX;
    sY = m->scrollY % m->tileY;
    if (sX < 0) sX += m->tileX;
    if (sY < 0) sY += m->tileY;

    dX = (((m->scrollX < 0) ? (m->scrollX - m->tileX + 1) : m->scrollX) / m->tileX) % m->mapSizeX;
    mY = (((m->scrollY < 0) ? (m->scrollY - m->tileY + 1) : m->scrollY) / m->tileY) % m->mapSizeY;

    if (dX < 0) dX += m->mapSizeX;
    if (mY < 0) mY += m->mapSizeY;

    yTile = -sY;

    // Handle different formats
    switch (m->format) {
        case OSL_MF_U16:
            for (y = 0; y < dsY; y++) {
                bY = m->mapSizeX * mY;
                mX = dX;
                xTile = -sX;
                vertices = (OSL_FAST_VERTEX*)sceGuGetMemory(dsX * 2 * sizeof(OSL_FAST_VERTEX));
                nbVertices = 0;

                for (x = 0; x < dsX; x++) {
                    v = map[bY + mX];

                    if (v || firstTileOpaque) {
                        if (tilesPerLineOpt) {
                            // Use optimized bit-shifting for tile index calculation
                            vertices[nbVertices].u = (v & ((1 << tilesPerLineOpt) - 1)) * m->tileX;
                            vertices[nbVertices].v = (v >> tilesPerLineOpt) * m->tileY;
                        } else {
                            // Fallback to regular division/modulo for tile index calculation
                            vertices[nbVertices].u = (v % tilesPerLine) * m->tileX;
                            vertices[nbVertices].v = (v / tilesPerLine) * m->tileY;
                        }

                        vertices[nbVertices].x = xTile;
                        vertices[nbVertices].y = yTile;
                        vertices[nbVertices].z = 0;
                        vertices[nbVertices + 1].u = vertices[nbVertices].u + m->tileX;
                        vertices[nbVertices + 1].v = vertices[nbVertices].v + m->tileY;
                        vertices[nbVertices + 1].x = vertices[nbVertices].x + m->tileX;
                        vertices[nbVertices + 1].y = vertices[nbVertices].y + m->tileY;
                        vertices[nbVertices + 1].z = 0;

                        nbVertices += 2;
                    }

                    xTile += m->tileX;
                    mX++;
                    if (mX >= m->mapSizeX)
                        mX -= m->mapSizeX;
                }

                if (nbVertices > 0)
                    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, nbVertices, 0, vertices);

                mY++;
                if (mY >= m->mapSizeY)
                    mY -= m->mapSizeY;
                yTile += m->tileY;
            }
            break;

        case OSL_MF_U16_GBA:
            for (y = 0; y < dsY; y++) {
                bY = m->mapSizeX * mY;
                mX = dX;
                xTile = -sX;
                vertices = (OSL_FAST_VERTEX*)sceGuGetMemory(dsX * 2 * sizeof(OSL_FAST_VERTEX));
                nbVertices = 0;

                for (x = 0; x < dsX; x++) {
                    int flags;
                    v = map[bY + mX];

                    // Extract GBA flags (flipping, palette, etc.)
                    flags = v & ~((1 << m->addit1) - 1);
                    v &= ((1 << m->addit1) - 1);

                    if (v || firstTileOpaque) {
                        if (tilesPerLineOpt) {
                            // Use optimized bit-shifting for tile index calculation
                            vertices[nbVertices].u = (v & ((1 << tilesPerLineOpt) - 1)) * m->tileX;
                            vertices[nbVertices].v = (v >> tilesPerLineOpt) * m->tileY;
                        } else {
                            // Fallback to regular division/modulo for tile index calculation
                            vertices[nbVertices].u = (v % tilesPerLine) * m->tileX;
                            vertices[nbVertices].v = (v / tilesPerLine) * m->tileY;
                        }

                        vertices[nbVertices].x = xTile;
                        vertices[nbVertices].y = yTile;
                        vertices[nbVertices].z = 0;
                        vertices[nbVertices + 1].u = vertices[nbVertices].u + m->tileX;
                        vertices[nbVertices + 1].v = vertices[nbVertices].v + m->tileY;
                        vertices[nbVertices + 1].x = vertices[nbVertices].x + m->tileX;
                        vertices[nbVertices + 1].y = vertices[nbVertices].y + m->tileY;
                        vertices[nbVertices + 1].z = 0;

                        // Handle horizontal flipping
                        if (flags & (1 << m->addit1)) {
                            int tmp = vertices[nbVertices].u;
                            vertices[nbVertices].u = vertices[nbVertices + 1].u;
                            vertices[nbVertices + 1].u = tmp;
                        }

                        // Handle vertical flipping
                        if (flags & (1 << (m->addit1 + 1))) {
                            int tmp = vertices[nbVertices].v;
                            vertices[nbVertices].v = vertices[nbVertices + 1].v;
                            vertices[nbVertices + 1].v = tmp;
                        }

                        nbVertices += 2;
                    }

                    xTile += m->tileX;
                    mX++;
                    if (mX >= m->mapSizeX)
                        mX -= m->mapSizeX;
                }

                if (nbVertices > 0)
                    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, nbVertices, 0, vertices);

                mY++;
                if (mY >= m->mapSizeY)
                    mY -= m->mapSizeY;
                yTile += m->tileY;
            }
            break;

        default:
            oslFatalError("Unsupported map format");
            break;
    }
}

void oslDeleteMap(OSL_MAP *m) {
    if (m) {
        free(m);
    }
}
