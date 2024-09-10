#include "oslib.h"

// Constants for screen dimensions and tiles
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272
#define TILE_SIZE 8
#define MAX_FADE 31
#define MAX_FRAME_COUNT 220
#define MAX_INITIAL_FRAME 120

// Helper macros for min and max
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

// Calculate tile grid dimensions
#define TILE_WIDTH (SCREEN_WIDTH / TILE_SIZE)
#define TILE_HEIGHT (SCREEN_HEIGHT / TILE_SIZE)

// Forward declarations of functions
void handlePhase(float *y, float *vy, int *phase, int logoSizeY);
void updateTileSpeeds(float speeds[TILE_HEIGHT][TILE_WIDTH], float positions[TILE_HEIGHT][TILE_WIDTH]);
void drawLogo(OSL_IMAGE *logo, float y);

static void LogoDrawTiles(OSL_IMAGE *img, float positions[TILE_HEIGHT][TILE_WIDTH])
{
    OSL_FAST_VERTEX *vertices;
    int nbVertices, x, y;

    oslSetTexture(img);

    for (y = 0; y < TILE_HEIGHT; y++)
    {
        vertices = (OSL_FAST_VERTEX*)sceGuGetMemory(TILE_WIDTH * 2 * sizeof(OSL_FAST_VERTEX));
        nbVertices = 0;
        for (x = 0; x < TILE_WIDTH; x++)
        {
            vertices[nbVertices].u = x * TILE_SIZE;
            vertices[nbVertices].v = y * TILE_SIZE;
            vertices[nbVertices].x = x * TILE_SIZE;
            vertices[nbVertices].y = positions[y][x];
            vertices[nbVertices].z = 0;
            nbVertices++;

            vertices[nbVertices].u = x * TILE_SIZE + TILE_SIZE;
            vertices[nbVertices].v = y * TILE_SIZE + TILE_SIZE;
            vertices[nbVertices].x = x * TILE_SIZE + TILE_SIZE;
            vertices[nbVertices].y = positions[y][x] + TILE_SIZE;
            vertices[nbVertices].z = 0;
            nbVertices++;
        }

        // Draw the vertices
        if (nbVertices > 0)
            sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, nbVertices, 0, vertices);
    }
}

int oslShowSplashScreen2()
{
    OSL_IMAGE *logo, *temp;
    float y = -1.0f, vy = 1.0f;
    int skip = 0, frameNb = 0, phase = 1, fade = 0;
    float vyList[TILE_WIDTH];
    float speeds[TILE_HEIGHT][TILE_WIDTH];
    float positions[TILE_HEIGHT][TILE_WIDTH];

    // Load the logo image
    temp = oslLoadImageFile("logo/neoflash.png", OSL_IN_RAM, OSL_PF_5650);
    if (!temp)
        return 0;

    logo = oslCreateSwizzledImage(temp, OSL_IN_VRAM);
    oslDeleteImage(temp);
    if (!logo)
        return 0;

    // Initialize speed and position arrays
    for (int i = 0; i < TILE_WIDTH; i++)
        vyList[i] = (rand() % 7500) / 10000.f + 0.25f;

    for (int j = 0; j < TILE_HEIGHT; j++)
        for (int i = 0; i < TILE_WIDTH; i++)
        {
            speeds[j][i] = vyList[i] - (TILE_HEIGHT - j) * ((rand() % 1000) / 10000.0f + 0.04f);
            positions[j][i] = j * TILE_SIZE;
        }

    // Splash screen loop
    while (!osl_quit && fade < MAX_FADE && frameNb < MAX_FRAME_COUNT)
    {
        oslReadKeys();

        // Handle the phases of the animation
        if (phase < 9)
        {
            handlePhase(&y, &vy, &phase, logo->sizeY);
        }
        else
        {
            y = 0;
            frameNb++;
        }

        // Adjust tile positions after initial frames
        if (frameNb > MAX_INITIAL_FRAME)
        {
            updateTileSpeeds(speeds, positions);
        }

        // Handle fade effect and user input
        if (fade > 0)
            fade++;

        if ((osl_keys->pressed.value & (OSL_KEYMASK_START | OSL_KEYMASK_CIRCLE | OSL_KEYMASK_CROSS)) && fade == 0)
            fade = 1;

        // Drawing the splash screen
        if (!skip)
        {
            oslStartDrawing();
            oslClearScreen(0);

            if (frameNb <= MAX_INITIAL_FRAME)
            {
                drawLogo(logo, y);
            }
            else
            {
                LogoDrawTiles(logo, positions);
            }

            if (fade > 0)
            {
                oslSetAlpha(OSL_FX_RGBA, 0);
                oslDrawFillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, RGBA(0, 0, 0, fade << 3));
            }

            oslEndDrawing();
        }

        // Sync frame rate
        skip = oslSyncFrameEx(1, 4, 0);
    }

    oslDeleteImage(logo);
    return 1;
}

// Function to handle phase transitions
void handlePhase(float *y, float *vy, int *phase, int logoSizeY)
{
    *y += *vy;
    if (*y + logoSizeY >= SCREEN_HEIGHT)
    {
        if (*vy > 0)
            *vy -= 0.8f;
        else
            *vy -= 0.4f;

        if (*phase % 2 == 1)
            (*phase)++;
    }
    else if (*phase == 1)
    {
        *vy += 0.4f;
    }
    else
    {
        if (*vy > 0)
            *vy += 0.4f;
        else
            *vy += 0.8f;

        if (*phase % 2 == 0)
            (*phase)++;
    }
}

// Function to update tile speeds and positions
void updateTileSpeeds(float speeds[TILE_HEIGHT][TILE_WIDTH], float positions[TILE_HEIGHT][TILE_WIDTH])
{
    for (int j = 0; j < TILE_HEIGHT; j++)
        for (int i = 0; i < TILE_WIDTH; i++)
        {
            speeds[j][i] += 0.15f;
            if (speeds[j][i] > 0.0f)
                positions[j][i] += speeds[j][i];
        }
}

// Function to draw the logo with stretch effect
void drawLogo(OSL_IMAGE *logo, float y)
{
    logo->y = y;
    if (y < 0)
    {
        logo->stretchY = logo->sizeY;
        logo->stretchX = logo->sizeX;
        logo->x = 0;
    }
    else
    {
        logo->stretchY = logo->sizeY - y;
        logo->stretchX = logo->sizeX + y;
        logo->x = -y / 2;
    }
    oslDrawImage(logo);
}
