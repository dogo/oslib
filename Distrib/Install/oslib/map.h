/**
 * @file map.h
 * @brief Map handling functions and structures in OSLib.
 *
 * This header defines the structures and functions used to create, manage,
 * and draw maps in the OSLib framework. Maps are composed of tiles from a tileset
 * image, and this file provides functions to manipulate these maps.
 */

#ifndef _OSL_MAP_H_
#define _OSL_MAP_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup maps Maps
 *  @brief Map functions in OSLib.
 *  @{
 */

/**
 * @brief Structure representing a map.
 *
 * The `OSL_MAP` structure holds all the necessary information about a map,
 * including its tileset image, map data, scrolling values, tile sizes,
 * and the format of the map.
 */
typedef struct {
	OSL_IMAGE *img;   /**< Pointer to the tileset image used by the map. */
	void *map;        /**< Pointer to the raw binary map data. */
	int scrollX;      /**< Horizontal scroll position (in pixels). */
	int scrollY;      /**< Vertical scroll position (in pixels). */
	int tileX;        /**< Width of a tile in the tileset (in pixels). */
	int tileY;        /**< Height of a tile in the tileset (in pixels). */
	int drawSizeX;    /**< Width of the drawing area (in pixels). */
	int drawSizeY;    /**< Height of the drawing area (in pixels). */
	int mapSizeX;     /**< Width of the map (in tiles). */
	int mapSizeY;     /**< Height of the map (in tiles). */
	u8 format;        /**< Format of the map, defined by `OSL_MAP_FORMATS`. */
	u8 flags;         /**< Flags defining map properties, see `OSL_MAP_FLAGS`. */
	u8 addit1;        /**< Additional map data used for special formats. */
} OSL_MAP;

/**
 * @brief Enum representing the available map formats.
 *
 * This enum defines the possible formats a map can have, such as 16-bit formats
 * with different tile numbering schemes.
 */
enum OSL_MAP_FORMATS {
	OSL_MF_U16 = 1,   /**< 16-bit format. */
	OSL_MF_U16_GBA = 2, /**< 16-bit, 10 bits for the tile number, 2 bits for mirroring.
	                         Palette is unsupported. `addit1` holds the number of tile bits
	                         (default 10), and the next two bits are for mirroring
	                         (11 = horizontal, 12 = vertical). */
};

/**
 * @brief Enum representing internal map flags.
 *
 * These flags are used internally to define certain properties of the map, such
 * as transparency and drawing methods.
 */
typedef enum OSL_MAP_FLAGS {
	OSL_MF_SIMPLE = 1,           /**< Defines if the map is "simple" (`oslDrawMapSimple`).
	                                   Currently unsupported, do not use. */
	OSL_MF_TILE1_TRANSPARENT = 2, /**< Defines if the first tile is always transparent,
	                                    regardless of its pattern. */
} OSL_MAP_FLAGS;

/**
 * @brief Creates a new map.
 *
 * This function initializes a new map using the provided tileset image and raw
 * map data. It sets up the map's dimensions and format, returning a pointer to
 * the created `OSL_MAP` structure.
 *
 * @param img Pointer to the tileset image. The maximum size of images is 512x512 pixels.
 *            In the tileset image, tiles are arranged from left to right and top to bottom.
 * @param map_data Pointer to the raw binary map data. This is an array where each entry
 *                represents a tile number to be displayed.
 * @param tileX Width of a tile in the tileset (in pixels).
 * @param tileY Height of a tile in the tileset (in pixels).
 * @param mapSizeX Width of the map (in tiles).
 * @param mapSizeY Height of the map (in tiles).
 * @param map_format Format of the map, one of the `OSL_MAP_FORMATS` values.
 *
 * @return Pointer to the created `OSL_MAP` structure, or `NULL` in case of error.
 */
extern OSL_MAP *oslCreateMap(OSL_IMAGE *img, void *map_data, int tileX, int tileY, int mapSizeX, int mapSizeY, int map_format);

/**
 * @brief Draws a map on the screen.
 *
 * This function renders the map on the screen using the properties defined
 * in the `OSL_MAP` structure, such as scroll positions and tile sizes.
 *
 * @param m Pointer to the `OSL_MAP` structure to be drawn.
 */
extern void oslDrawMap(OSL_MAP *m);

/**
 * @brief Draws a map using a simple method (deprecated).
 *
 * This function is provided for backward compatibility. It draws a map
 * similarly to `oslDrawMap`, but is not recommended for use in new code.
 *
 * @param m Pointer to the `OSL_MAP` structure to be drawn.
 *
 * @deprecated This function is deprecated and should not be used.
 */
extern void oslDrawMapSimple(OSL_MAP *m);

/**
 * @brief Deletes a map.
 *
 * This function frees the resources associated with a map. Note that the raw
 * map data passed to `oslCreateMap` is not freed by this function and must be
 * managed separately by the user.
 *
 * @param m Pointer to the `OSL_MAP` structure to be deleted.
 *
 * Example:
 * @code
 * unsigned short *mapData;
 * OSL_MAP *map;
 * // Allocate memory for map data
 * mapData = malloc(128 * 64 * sizeof(*mapData));
 * // Create a map with this data
 * map = oslCreateMap(anImage, mapData, 16, 16, 128, 64, OSL_MF_U16);
 * // Delete the map
 * oslDeleteMap(map);
 * // Free the map data
 * free(mapData);
 * @endcode
 */
extern void oslDeleteMap(OSL_MAP *m);

/** @} */ // end of maps

#ifdef __cplusplus
}
#endif

#endif /* _OSL_MAP_H_ */
