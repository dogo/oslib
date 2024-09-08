#include "oslib.h"

// Define constants for UI layout
#define BUTTON_WIDTH 80
#define BUTTON_HEIGHT 21
#define BUTTON_SPACING 90
#define FRAME_BORDER_WIDTH 2

// State values for button frame
#define BUTTON_STATE_NORMAL 0
#define BUTTON_STATE_PRESSED 1

// Color constants
#define COLOR_FRAME_LIGHT RGB(208,208,208)
#define COLOR_FRAME_DARK RGB(188,188,188)
#define COLOR_SHADOW RGB(128,128,128)
#define COLOR_REFLECT RGB(255,255,255)
#define COLOR_TITLE_BAR_START RGB(0,0,128)
#define COLOR_TITLE_BAR_END RGB(16,132,208)
#define COLOR_TEXT RGB(255,255,255)
#define COLOR_BACKGROUND RGB(0,0,0)

void oslUIDrawButtonFrame(int x0, int y0, int w, int h, int state)
{
    int x1 = x0 + w;
    int y1 = y0 + h;

    // Draw gradient rectangle
    oslDrawGradientRect(x0, y0, x1, y1, COLOR_FRAME_LIGHT, COLOR_FRAME_DARK, COLOR_FRAME_DARK, COLOR_FRAME_LIGHT);

    if (state == BUTTON_STATE_PRESSED)
    {
        // Draw pressed state frame
        oslDrawRect(x0, y0, x1, y1, RGB(128,128,128));
        x0++, y0++, w -= 2, h -= 2;
    }

    // Draw border and shadow
    oslDrawLine(x1 - 1, y0, x1 - 1, y1, RGB(0,0,0));
    oslDrawLine(x0, y1 - 1, x1, y1 - 1, RGB(0,0,0));
    oslDrawLine(x1 - 2, y0 + 1, x1 - 2, y1 - 1, COLOR_SHADOW);
    oslDrawLine(x0 + 1, y1 - 2, x1 - 2, y1 - 2, COLOR_SHADOW);

    // Draw reflection
    oslDrawLine(x0 + 1, y0 + 1, x1 - 2, y0 + 1, COLOR_REFLECT);
    oslDrawLine(x0 + 1, y0 + 1, x0 + 1, y1 - 2, COLOR_REFLECT);
}

unsigned int oslMessageBox(const char *text, const char *title, unsigned int flags)
{
    typedef struct {
        int key, option;
        char text[32];
    } OSL_BUTTON_INFO;

    // Define button positions
    const int x0 = 240 - 150, y0 = 136 - 60;
    const int x1 = 240 + 150, y1 = 136 + 60;
    int btX, nbButtons, i;
    OSL_BUTTON_INFO buttons[3];
    unsigned char c;
    const unsigned char keys[] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 'L', 'R', ' ', ' ', '^', 'o', 'x', '#', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    OSL_IMAGE *curBuf = oslGetDrawBuffer();
    OSL_FONT *ft = osl_curFont;
    int dither = osl_ditheringEnabled;

    // Start drawing
    oslStartDrawing();

    // Load font if not already loaded
    if (!osl_sceFont)
        osl_sceFont = oslLoadFont(&osl_sceFontInfo);
    oslSetFont(osl_sceFont);

    // Set draw buffer and dithering
    oslSetDrawBuffer(OSL_SECONDARY_BUFFER);
    oslSetDithering(1);

    // Draw message box frame
    oslUIDrawButtonFrame(x0, y0, x1 - x0, y1 - y0, BUTTON_STATE_NORMAL);

    // Draw title bar
    oslDrawGradientRect(x0 + 3, y0 + 3, x1 - 3, y0 + 3 + 13, COLOR_TITLE_BAR_START, COLOR_TITLE_BAR_END, COLOR_TITLE_BAR_START, COLOR_TITLE_BAR_END);

    // Draw title text
    oslSetBkColor(RGBA(0, 0, 0, 0));
    oslSetTextColor(COLOR_TEXT);
    oslSetScreenClipping(x0 + 3, y0 + 3, x1 - 3, y0 + 3 + 13);
    oslDrawString(x0 + 8, y0 + 6, title);
    oslResetScreenClipping();

    // Draw message text
    oslSetTextColor(RGB(0, 0, 0));
    oslDrawTextBox(x0 + 8, y0 + 22, x1 - 4, y1 - 30, text, 0);

    // Process buttons
    for (i = 0; i < 3; i++)
    {
        if ((flags & 511) == 0)
            break;

        buttons[i].key = (flags & 31);
        if (buttons[i].key >= 1 && buttons[i].key <= 24) {
			c = keys[buttons[i].key - 1];
		} else {
			c = ' ';
		}
        buttons[i].option = (flags >> 5) & 15;

        switch (buttons[i].option)
        {
            case OSL_MB_OK:
                sprintf(buttons[i].text, "%c Ok", c);
                break;
            case OSL_MB_CANCEL:
                sprintf(buttons[i].text, "%c Cancel", c);
                break;
            case OSL_MB_YES:
                sprintf(buttons[i].text, "%c Yes", c);
                break;
            case OSL_MB_NO:
                sprintf(buttons[i].text, "%c No", c);
                break;
            case OSL_MB_QUIT:
                sprintf(buttons[i].text, "%c Quit", c);
                break;
            default:
                strcpy(buttons[i].text, "");
        }
        flags >>= 9;
    }
    nbButtons = i;
    btX = ((x0 + x1) / 2) - (BUTTON_SPACING * nbButtons) / 2 + 5;

    // Draw buttons
    for (i = 0; i < nbButtons; i++)
    {
        oslUIDrawButtonFrame(btX + BUTTON_SPACING * i, y1 - 30, BUTTON_WIDTH, BUTTON_HEIGHT, BUTTON_STATE_PRESSED);
        oslDrawString(btX + 10 + BUTTON_SPACING * i, y1 - 23, buttons[i].text);
    }

    // Restore previous settings
    oslSetFont(ft);
    oslSetDithering(dither);

    // Event loop
    while (!osl_quit)
    {
        oslReadKeys();
        for (i = 0; i < nbButtons; i++)
        {
            if (osl_keys->pressed.value & (1 << (buttons[i].key - 1)))
                break;
        }
        if (i < nbButtons)
        {
            if (buttons[i].option == OSL_MB_QUIT)
                oslQuit();
            else
            {
                oslSetDrawBuffer(curBuf);
                oslReadKeys();
                return buttons[i].option;
            }
        }
        if (nbButtons == 0)
        {
            if (osl_keys->pressed.cross)
            {
                oslSetDrawBuffer(curBuf);
                break;
            }
        }
        sceDisplayWaitVblankStart();
    }
    oslReadKeys();
    return 0;
}
