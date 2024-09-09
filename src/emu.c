#include "oslib.h"
#include "../PC_Main/GL/glext.h"
#include "../PC_Main/OpenGL.h"
#include <math.h>

// Constants
#define MAX_VRAM_TEMP_SIZE (1 << 20)  // 1 MB of temporary VRAM space

// VRAM and texture management
u8 *emuVramTempMemoryPtr = NULL;
u32 emuCurrentTexturePixelFormat = OSL_PF_8888, emuCurrentPalettePixelFormat = OSL_PF_8888;
u32 emuCurrentClearColor = 0xFFFFFFFF;
const int emu_pixelFormats[] = { GL_RGB5, GL_RGB5_A1, GL_RGBA4, GL_RGBA8, 0, 0 };
const int emu_pixelPhysFormats[] = { GL_UNSIGNED_SHORT_5_6_5_REV, GL_UNSIGNED_SHORT_1_5_5_5_REV, GL_UNSIGNED_SHORT_4_4_4_4_REV, GL_UNSIGNED_BYTE, 0, 0 };
u32 emuCurrentTexSizeX = 0, emuCurrentTexSizeY = 0, emuCurrentTextureState = 0, emuCurrentAmbientColor = 0xFFFFFFFF;
void *emuCurrentPalette = NULL, *emuCurrentTextureSourceData = NULL;
u32 emuCurrentTextureCache[512][512];
u32 *emuTempTextureMemory = NULL;
int emuSkipFrame = 0, emuDoubleBuffer = 1;
u8 *OSL_UVRAM_BASE = NULL;  // Base address for 2MB VRAM allocation
u32 emuPaletteCache[256];
u32 emuIsAlphaWriteEnabled = 0;
s64 emuCounterFrequency = 0, emuCounterBaseValue = 0;
int emuCurrentTextureNo = 0;
GLuint emuGlTexture[EMU_NB_TEXTURES] = {0};

// Shader-related function pointers
PFNGLBLENDCOLOREXTPROC glBlendColor = NULL;
PFNGLBLENDEQUATIONPROC glBlendEquation = NULL;
GLhandleARB emuVertexShaderID = 0, emuFragmentShaderID = 0, emuProgramObjectID = 0;

// Shader function pointers
PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB = NULL;
PFNGLSHADERSOURCEARBPROC glShaderSourceARB = NULL;
PFNGLCOMPILESHADERARBPROC glCompileShaderARB = NULL;
PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB = NULL;
PFNGLATTACHOBJECTARBPROC glAttachObjectARB = NULL;
PFNGLLINKPROGRAMARBPROC glLinkProgramARB = NULL;
PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB = NULL;
PFNGLUNIFORM1IARBPROC glUniform1iARB = NULL;
PFNGLUNIFORM1FARBPROC glUniform1fARB = NULL;
PFNGLUNIFORM2FARBPROC glUniform2fARB = NULL;
PFNGLUNIFORM3FARBPROC glUniform3fARB = NULL;
PFNGLUNIFORM4FARBPROC glUniform4fARB = NULL;
PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB = NULL;
PFNGLDETACHOBJECTARBPROC glDetachObjectARB = NULL;
PFNGLDELETEOBJECTARBPROC glDeleteObjectARB = NULL;

// Emulator parameters
int emuReuseTextures = 0;   // Texture reuse mode
int emuUsePixelShaders = 0; // Shader usage flag
int emuFrameSkipMode = 0;   // 0: PC timing, 1: slow PC, 2: frame skip not supported
int emuPauseGame = 0;

// GLSL Initialization function
int emuShadersInitGLSL() {
    const char* szGLExtensions = (const char*)glGetString(GL_EXTENSIONS);

    if (!szGLExtensions) {
        MessageBox(NULL, "Failed to retrieve OpenGL extensions.", "Error", MB_OK);
        return 0;
    }

    // Check for necessary GLSL extensions
    if (!strstr(szGLExtensions, "GL_ARB_shader_objects")) {
        MessageBox(NULL, "GL_ARB_shader_objects extension not supported!", "Error", MB_OK);
        return 0;
    }

    if (!strstr(szGLExtensions, "GL_ARB_shading_language_100")) {
        MessageBox(NULL, "GL_ARB_shading_language_100 extension not supported!", "Error", MB_OK);
        return 0;
    }

    // Load shader-related OpenGL functions using wglGetProcAddress
    glCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC)wglGetProcAddress("glCreateShaderObjectARB");
    glShaderSourceARB = (PFNGLSHADERSOURCEARBPROC)wglGetProcAddress("glShaderSourceARB");
    glCompileShaderARB = (PFNGLCOMPILESHADERARBPROC)wglGetProcAddress("glCompileShaderARB");
    glCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC)wglGetProcAddress("glCreateProgramObjectARB");
    glAttachObjectARB = (PFNGLATTACHOBJECTARBPROC)wglGetProcAddress("glAttachObjectARB");
    glLinkProgramARB = (PFNGLLINKPROGRAMARBPROC)wglGetProcAddress("glLinkProgramARB");
    glUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC)wglGetProcAddress("glUseProgramObjectARB");
    glUniform1iARB = (PFNGLUNIFORM1IARBPROC)wglGetProcAddress("glUniform1iARB");
    glUniform1fARB = (PFNGLUNIFORM1FARBPROC)wglGetProcAddress("glUniform1fARB");
    glUniform2fARB = (PFNGLUNIFORM2FARBPROC)wglGetProcAddress("glUniform2fARB");
    glUniform3fARB = (PFNGLUNIFORM3FARBPROC)wglGetProcAddress("glUniform3fARB");
    glUniform4fARB = (PFNGLUNIFORM4FARBPROC)wglGetProcAddress("glUniform4fARB");
    glGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC)wglGetProcAddress("glGetUniformLocationARB");
    glDetachObjectARB = (PFNGLDETACHOBJECTARBPROC)wglGetProcAddress("glDetachObjectARB");
    glDeleteObjectARB = (PFNGLDELETEOBJECTARBPROC)wglGetProcAddress("glDeleteObjectARB");

    return 1;
}

char* emuShadersLoadTextFile(char* strFile) {
    FILE* f;
    char* dest = NULL;
    long fileSize;

    // Open the file
    f = fopen(strFile, "r");
    if (!f) {
        printf("Error: Failed to open shader file %s.\n", strFile);
        return NULL;
    }

    // Get the file size
    fseek(f, 0, SEEK_END);
    fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fileSize <= 0) {
        printf("Error: Shader file %s is empty.\n", strFile);
        fclose(f);
        return NULL;
    }

    // Allocate enough memory to hold the entire file + null terminator
    dest = (char*)malloc(fileSize + 1);
    if (!dest) {
        printf("Error: Failed to allocate memory for shader file %s.\n", strFile);
        fclose(f);
        return NULL;
    }

    // Read the entire file into the buffer
    fread(dest, 1, fileSize, f);
    dest[fileSize] = '\0';  // Null terminate the string

    fclose(f);
    return dest;
}

void emuShadersRelease() {
    if (emuProgramObjectID) {
        if (emuVertexShaderID) {
            glDetachObjectARB(emuProgramObjectID, emuVertexShaderID);
            glDeleteObjectARB(emuVertexShaderID);
            emuVertexShaderID = NULL;
        }

        if (emuFragmentShaderID) {
            glDetachObjectARB(emuProgramObjectID, emuFragmentShaderID);
            glDeleteObjectARB(emuFragmentShaderID);
            emuFragmentShaderID = NULL;
        }

        glDeleteObjectARB(emuProgramObjectID);
        emuProgramObjectID = NULL;
    }
}

// Initialize shaders
void emuShadersInitShaders(const char* szVertexFile, const char* szFragmentFile) {
    char* szVShader = NULL;
    char* szFShader = NULL;

    // Ensure there's either a vertex or fragment shader file
    if (!szVertexFile && !szFragmentFile) return;

    // Release previous shaders/programs if they exist
    if (emuVertexShaderID || emuFragmentShaderID || emuProgramObjectID) {
        emuShadersRelease();
    }

    // Create vertex shader if vertex file is provided
    if (szVertexFile) {
        emuVertexShaderID = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
        szVShader = emuShadersLoadTextFile(szVertexFile);
    }

    // Create fragment shader if fragment file is provided
    if (szFragmentFile) {
        emuFragmentShaderID = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
        szFShader = emuShadersLoadTextFile(szFragmentFile);
    }

    // Set shader sources and compile them
    if (szVShader) {
        glShaderSourceARB(emuVertexShaderID, 1, (const GLcharARB**)&szVShader, NULL);
        glCompileShaderARB(emuVertexShaderID);
        // Check for compile errors in the vertex shader
        GLint compiled;
        glGetObjectParameterivARB(emuVertexShaderID, GL_OBJECT_COMPILE_STATUS_ARB, &compiled);
        if (!compiled) {
            // Handle error, log it, etc.
            printf("Error: Vertex shader failed to compile.\n");
        }
        free(szVShader); // Free the shader source after compilation
    }

    if (szFShader) {
        glShaderSourceARB(emuFragmentShaderID, 1, (const GLcharARB**)&szFShader, NULL);
        glCompileShaderARB(emuFragmentShaderID);
        // Check for compile errors in the fragment shader
        GLint compiled;
        glGetObjectParameterivARB(emuFragmentShaderID, GL_OBJECT_COMPILE_STATUS_ARB, &compiled);
        if (!compiled) {
            // Handle error, log it, etc.
            printf("Error: Fragment shader failed to compile.\n");
        }
        free(szFShader); // Free the shader source after compilation
    }

    // Create the program object
    emuProgramObjectID = glCreateProgramObjectARB();

    // Attach the shaders to the program
    if (emuVertexShaderID) {
        glAttachObjectARB(emuProgramObjectID, emuVertexShaderID);
    }
    if (emuFragmentShaderID) {
        glAttachObjectARB(emuProgramObjectID, emuFragmentShaderID);
    }

    // Link the program
    glLinkProgramARB(emuProgramObjectID);
    // Check for linking errors
    GLint linked;
    glGetObjectParameterivARB(emuProgramObjectID, GL_OBJECT_LINK_STATUS_ARB, &linked);
    if (!linked) {
        // Handle link error, log it, etc.
        printf("Error: Shader program failed to link.\n");
    }

    // Use the program
    glUseProgramObjectARB(emuProgramObjectID);

    // Set the texture uniform
    int var = glGetUniformLocationARB(emuProgramObjectID, "tex");
    if (var != -1) {
        glUniform1iARB(var, 0); // Texture unit 0
    } else {
        printf("Warning: Uniform 'tex' not found in shader.\n");
    }
}

// Initialize emulation timing
void emuInitEmulation() {
    QueryPerformanceFrequency((LARGE_INTEGER*)&emuCounterFrequency);
    QueryPerformanceCounter((LARGE_INTEGER*)&emuCounterBaseValue);
}

// Initialize OpenGL state and textures
void emuInitGL() {
    int i;
    for (i = 0; i < EMU_NB_TEXTURES; i++) {
        glGenTextures(1, &emuGlTexture[i]);
        glBindTexture(GL_TEXTURE_2D, emuGlTexture[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    emuCurrentTextureNo = 0;
    emuCurrentTexSizeX = 0;
    emuCurrentTexSizeY = 0;
    emuCurrentTexturePixelFormat = -1;

    // Allocate temporary texture memory if not already done
    if (!emuTempTextureMemory) {
        emuTempTextureMemory = (u32*)malloc(512 * 512 * sizeof(u32));
    }

    // The PSP doesn't write to the alpha component
    emuIsAlphaWriteEnabled = 0;
    glColorMask(TRUE, TRUE, TRUE, FALSE);

    // VRAM allocation (2 MB)
    if (!OSL_UVRAM_BASE) {
        OSL_UVRAM_BASE = (u8*)malloc(2 << 20); // Allocate 2 MB
    }

    // Get function pointers for blend equations and colors
    glBlendEquation = (PFNGLBLENDEQUATIONPROC)wglGetProcAddress("glBlendEquation");
    glBlendColor = (PFNGLBLENDCOLOREXTPROC)wglGetProcAddress("glBlendColor");

    // Initialize shaders if pixel shaders are enabled
    if (emuUsePixelShaders) {
        emuShadersInitGLSL(); // Assuming this function initializes shader support
        emuShadersInitShaders(NULL, "psp.frag");
    }
}

// Kernel cache writeback functions (no-op for emulation)
void sceKernelDcacheWritebackInvalidateRange(void* p, int n) {}
void sceKernelDcacheWritebackRange(void* p, int n) {}
void sceKernelDcacheWritebackInvalidateAll() {}

typedef struct		{
	float u, v;
	//Weights not supported
	u32 color, colored;
	//Normal not supported
	float x, y, z;
} EMU_VERTEX;

// Function to set vertex color, applying ambient lighting if needed
void emuSetVertexColor(unsigned int color) {
    unsigned int col;

    if (!emuCurrentTextureState) {
        col = color; // Non-textured mode, use raw color
    } else {
        // Apply ambient lighting multiplication in textured mode
        col =
            (((u64)(color & 0xff000000) * (emuCurrentAmbientColor & 0xff000000)) >> 32) |
            (((u64)(color & 0xff0000) * (emuCurrentAmbientColor & 0xff0000)) >> 24) |
            (((color & 0xff00) * (emuCurrentAmbientColor & 0xff00)) >> 16) |
            (((color & 0xff) * (emuCurrentAmbientColor & 0xff)) >> 8);
    }

    // Set OpenGL color using the calculated color value
    glColor4ub(
        (GLubyte)(col),
        (GLubyte)(col >> 8),
        (GLubyte)(col >> 16),
        (GLubyte)(col >> 24)
    );
}

// Get the next vertex from the vertex buffer
const void* emuGetVertex(const void* vertices, int vtype, EMU_VERTEX* v) {
    const uint8_t* ptr = (const uint8_t*)vertices;
    uint32_t value;
    uint32_t align = 1; // Default alignment

    // Handle texture coordinates (u, v)
    value = vtype & GU_TEXTURE_BITS;
    switch (value) {
        case GU_TEXTURE_8BIT:
            v->u = (float)(*(int8_t*)ptr);  // 8-bit texture coordinates
            ptr += 1;
            v->v = (float)(*(int8_t*)ptr);
            ptr += 1;
            break;

        case GU_TEXTURE_16BIT:
            v->u = (float)(*(int16_t*)ptr); // 16-bit texture coordinates
            ptr += 2;
            v->v = (float)(*(int16_t*)ptr);
            ptr += 2;
            align = 2;
            break;

        case GU_TEXTURE_32BITF:
            v->u = *(float*)ptr;            // 32-bit float texture coordinates
            ptr += 4;
            v->v = *(float*)ptr;
            ptr += 4;
            align = 4;
            break;

        default:
            // No texture coordinates, set defaults
            v->u = 0.0f;
            v->v = 0.0f;
            break;
    }

    // Handle vertex color
    value = vtype & GU_COLOR_BITS;
    v->colored = value; // Store the fact that the vertex is colored
    switch (value) {
        case GU_COLOR_5650:
            v->color = oslConvertColor(0x8888, 0x5650, *(uint16_t*)ptr);  // 5650 format
            ptr += 2;
            align = (align < 2) ? 2 : align;
            break;

        case GU_COLOR_5551:
            v->color = oslConvertColor(0x8888, 0x5551, *(uint16_t*)ptr);  // 5551 format
            ptr += 2;
            align = (align < 2) ? 2 : align;
            break;

        case GU_COLOR_4444:
            v->color = oslConvertColor(0x8888, 0x4444, *(uint16_t*)ptr);  // 4444 format
            ptr += 2;
            align = (align < 2) ? 2 : align;
            break;

        case GU_COLOR_8888:
            v->color = *(uint32_t*)ptr;                                  // 8888 format
            ptr += 4;
            align = (align < 4) ? 4 : align;
            break;

        default:
            v->color = 0xFFFFFFFF;  // Fully opaque if no color specified
            break;
    }

    // Handle vertex coordinates (x, y, z)
    value = vtype & GU_VERTEX_BITS;
    switch (value) {
        case GU_VERTEX_8BIT:
            v->x = (float)(*(int8_t*)ptr);   // 8-bit vertex position
            ptr += 1;
            v->y = (float)(*(int8_t*)ptr);
            ptr += 1;
            v->z = (float)(*(int8_t*)ptr);
            ptr += 1;
            break;

        case GU_VERTEX_16BIT:
            v->x = (float)(*(int16_t*)ptr);  // 16-bit vertex position
            ptr += 2;
            v->y = (float)(*(int16_t*)ptr);
            ptr += 2;
            v->z = (float)(*(int16_t*)ptr);
            ptr += 2;
            align = (align < 2) ? 2 : align;
            break;

        case GU_VERTEX_32BITF:
            v->x = *(float*)ptr;             // 32-bit float vertex position
            ptr += 4;
            v->y = *(float*)ptr;
            ptr += 4;
            v->z = *(float*)ptr;
            ptr += 4;
            align = (align < 4) ? 4 : align;
            break;

        default:
            // No vertex specified, set defaults
            v->x = v->y = v->z = 0.0f;
            break;
    }

    // Handle 2D transformation (divide u, v by texture size)
    if (vtype & GU_TRANSFORM_2D) {
        v->u /= emuCurrentTexSizeX;
        v->v /= emuCurrentTexSizeY;
    }

    // Align to the next multiple of `align`
    uintptr_t offset = (uintptr_t)ptr - (uintptr_t)vertices;
    if (offset % align) {
        ptr += align - (offset % align);
    }

    return ptr;
}

// Draw an array of vertices
void sceGuDrawArray(int prim, int vtype, int count, const void* indices, const void* vertices) {
    EMU_VERTEX v[8];

    if (emuSkipFrame) {
        return;  // Skip drawing if the frame is flagged to be skipped
    }

    // GU_SPRITES case
    if (prim == GU_SPRITES) {
        while (count > 0) {
            // Fetch vertices in pairs and draw quads
            vertices = emuGetVertex(vertices, vtype, &v[0]);
            vertices = emuGetVertex(vertices, vtype, &v[1]);

            // Set the vertex color and draw the quad
            emuSetVertexColor(v[0].color);
            glBegin(GL_QUADS);
                glTexCoord2f(v[0].u, v[0].v); glVertex3f(v[0].x, v[0].y, 0);
                glTexCoord2f(v[1].u, v[0].v); glVertex3f(v[1].x, v[0].y, 0);
                glTexCoord2f(v[1].u, v[1].v); glVertex3f(v[1].x, v[1].y, 0);
                glTexCoord2f(v[0].u, v[1].v); glVertex3f(v[0].x, v[1].y, 0);
            glEnd();

            count -= 2;
        }
    }
    // GU_TRIANGLE_STRIP case
    else if (prim == GU_TRIANGLE_STRIP) {
        while (count > 0) {
            // Fetch 4 vertices and draw a triangle strip
            vertices = emuGetVertex(vertices, vtype, &v[0]);
            vertices = emuGetVertex(vertices, vtype, &v[1]);
            vertices = emuGetVertex(vertices, vtype, &v[2]);
            vertices = emuGetVertex(vertices, vtype, &v[3]);

            glBegin(GL_TRIANGLE_STRIP);
                for (int i = 0; i < 4; i++) {
                    emuSetVertexColor(v[i].color);
                    glTexCoord2f(v[i].u, v[i].v);
                    glVertex3f(v[i].x, v[i].y, v[i].z);
                }
            glEnd();

            count -= 4;
        }
    }
    // GU_LINES case
    else if (prim == GU_LINES) {
        glBegin(GL_LINES);
        while (count > 0) {
            // Fetch 2 vertices at a time and draw lines
            vertices = emuGetVertex(vertices, vtype, &v[0]);
            vertices = emuGetVertex(vertices, vtype, &v[1]);

            for (int i = 0; i < 2; i++) {
                emuSetVertexColor(v[i].color);
                glTexCoord2f(v[i].u, v[i].v);
                glVertex3f(v[i].x, v[i].y, v[i].z);
            }

            count -= 2;
        }
        glEnd();
    }
}

// Memory alignment function
void* memalign(size_t alignment, size_t size) {
    return malloc(size);
}

// Copy memory using DMA (or in this case, a simple memcpy)
int sceDmacMemcpy(void* dest, const void* source, unsigned int size) {
    return (int)memcpy(dest, source, size);
}

// Enable OpenGL state
void sceGuEnable(int state) {
    if (state >= 0) {
        glEnable(state);
    }
    if (state == GU_TEXTURE_2D) {
        emuCurrentTextureState = 1;  // Track texture state for the emulator
    }
}

// Disable OpenGL state
void sceGuDisable(int state) {
    if (state >= 0) {
        glDisable(state);
    }
    if (state == GU_TEXTURE_2D) {
        emuCurrentTextureState = 0;  // Track texture state for the emulator
    }
}

// Alpha function, mask not supported
void sceGuAlphaFunc(int func, int value, int mask) {
    // Mask is ignored
    glAlphaFunc(func, (GLclampf)(value / 255.0));
}

// Set ambient color
void sceGuAmbientColor(unsigned int color) {
    emuCurrentAmbientColor = color;
    glColor4ub((GLubyte)(color), (GLubyte)(color >> 8), (GLubyte)(color >> 16), (GLubyte)(color >> 24));
}

// Set blend function
void sceGuBlendFunc(int op, int src, int dest, unsigned int srcfix, unsigned int destfix) {
    if (glBlendEquation && glBlendColor) {
        glBlendEquation(op);

        // Handle fixed source and destination
        if (dest == GU_FIX) {
            glBlendColor(destfix / 255.0f, (destfix >> 8) / 255.0f, (destfix >> 16) / 255.0f, (destfix >> 24) / 255.0f);
        } else if (src == GU_FIX) {
            glBlendColor(srcfix / 255.0f, (srcfix >> 8) / 255.0f, (srcfix >> 16) / 255.0f, (srcfix >> 24) / 255.0f);
        }

        glBlendFunc(src, dest);
    } else {
        // Fallback for systems without full blend support
        if (dest == GU_FIX) {
            dest = (destfix & 0x00ffffff) == 0x00ffffff ? GL_ONE : GL_ZERO;
        }
        if (src == GU_FIX) {
            src = (srcfix & 0x00ffffff) == 0x00ffffff ? GL_ONE : GL_ZERO;
        }

        if (op == GU_ADD) {
            glBlendFunc(src, dest);
        }
    }
}

void sceGuTexFunc(int tfx, int tcc) {
	//NOT NECESSARY???
}

void sceGuColorFunc(int func, unsigned int color, unsigned int mask) {
	//IMPOSSIBLE
}

// Temporary memory allocation
void* sceGuGetMemory(int size) {
    if (!emuVramTempMemoryPtr) {
        emuVramTempMemoryPtr = (unsigned int*)malloc(MAX_VRAM_TEMP_SIZE);
    }
    return emuVramTempMemoryPtr;
}

// Convert palette texture to 8888 format
void emuConvertPaletteTexture(int width, int height, const void* tbp) {
    unsigned int* pal = (unsigned int*)emuCurrentPalette;
    unsigned int* tex = (unsigned int*)emuCurrentTextureCache;

    int x, y;
    if (!tbp) {
        fprintf(stderr, "Error: Null texture buffer pointer in emuConvertPaletteTexture.\n");
        return;
    }

    switch (emuCurrentTexturePixelFormat) {
        case GU_PSM_T4: {
            unsigned char* data = (unsigned char*)tbp;
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x += 2) {
                    tex[y * width + x] = pal[*data & 0xF];
                    tex[y * width + x + 1] = pal[(*data >> 4) & 0xF];
                    data++;
                }
            }
            break;
        }
        case GU_PSM_T8: {
            unsigned char* data = (unsigned char*)tbp;
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    tex[y * width + x] = pal[*data++];
                }
            }
            break;
        }
        case GU_PSM_5650: {
            unsigned short* data = (unsigned short*)tbp;
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    unsigned short col = *data++;
                    tex[y * width + x] = ((col & 0x1F) << 3) |
                                         (((col >> 5) & 0x3F) << 10) |
                                         (((col >> 11) & 0x1F) << 19) |
                                         0xFF000000;  // Add alpha channel
                }
            }
            break;
        }
        case GU_PSM_5551: {
            unsigned short* data = (unsigned short*)tbp;
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    unsigned short col = *data++;
                    tex[y * width + x] = ((col & 0x1F) << 3) |
                                         (((col >> 5) & 0x1F) << 11) |
                                         (((col >> 10) & 0x1F) << 19) |
                                         ((col >> 15) ? 0xFF000000 : 0x00000000);  // Alpha bit
                }
            }
            break;
        }
        case GU_PSM_4444: {
            unsigned short* data = (unsigned short*)tbp;
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    unsigned short col = *data++;
                    tex[y * width + x] = ((col & 0xF) << 4) |
                                         (((col >> 4) & 0xF) << 12) |
                                         (((col >> 8) & 0xF) << 20) |
                                         (((col >> 12) & 0xF) << 28);
                }
            }
            break;
        }
        default: {
            fprintf(stderr, "Unsupported texture format in emuConvertPaletteTexture.\n");
            break;
        }
    }
}

// Function to handle texture uploads
void sceGuTexImage(int mipmap, int width, int height, int tbw, const void* tbp) {
    int pf;                 // Pixel format to be used in OpenGL
    int needConvert = 0;    // Flag to determine if conversion is needed

    // Skip if the frame should be skipped
    if (emuSkipFrame) {
        return;
    }

    // Store the current texture data pointer
    emuCurrentTextureSourceData = (void*)tbp;

    // Check if the texture is using a palette format
    if (emuCurrentTexturePixelFormat == GU_PSM_T4 || emuCurrentTexturePixelFormat == GU_PSM_T8) {
        needConvert = 1;
    }

redo:
    if (needConvert) {
        // Convert the palette texture to 8888 format
        emuConvertPaletteTexture(width, height, tbp);
        tbp = emuCurrentTextureCache;
        pf = GU_PSM_8888; // After conversion, we use the 8888 format
    } else {
        pf = emuCurrentTexturePixelFormat;
    }

    // Ensure OpenGL has finished before modifying textures if not reusing
    if (!emuReuseTextures) {
        glFinish();
    }

    // Set the current texture size (width and height)
    emuCurrentTexSizeX = tbw;  // In PSP, tbw (texture buffer width) is used for texture width
    emuCurrentTexSizeY = height;

    // Bind the texture for OpenGL
    glBindTexture(GL_TEXTURE_2D, emuGlTexture[emuCurrentTextureNo]);

    // Upload texture data to OpenGL
    glTexImage2D(
        GL_TEXTURE_2D, mipmap, emu_pixelFormats[pf],
        emuCurrentTexSizeX, oslGetNextPower2(emuCurrentTexSizeY),
        0, GL_RGBA, emu_pixelPhysFormats[pf], tbp
    );

    // If there was an error, attempt to convert the texture to 8888 format
    if (glGetError() != GL_NO_ERROR) {
        if (pf != GU_PSM_8888) {
            needConvert = 1;
            goto redo; // Retry the conversion and texture upload with the 8888 format
        } else {
            // Log or handle errors if necessary
            fprintf(stderr, "OpenGL Error: Unable to set texture mode.\n");
        }
    }

    // Move to the next texture in the texture array
    emuCurrentTextureNo = (emuCurrentTextureNo + 1) % EMU_NB_TEXTURES;
}

// Function to set texture mode
void sceGuTexMode(int tpsm, int maxmips, int a2, int swizzle) {
    emuCurrentTexturePixelFormat = tpsm;
}

// Function to load a CLUT (Color Look-Up Table)
void sceGuClutLoad(int num_blocks, const void* cbp) {
    if (emuSkipFrame) {
        return;
    }

    if (emuCurrentPalettePixelFormat == OSL_PF_8888) {
        emuCurrentPalette = (void*)cbp;
    } else {
        // Convert palette to 8888 format
        emuCurrentPalette = (void*)emuPaletteCache;
        for (int i = 0; i < (num_blocks << 3); i++) {
            emuPaletteCache[i] = oslConvertColor(OSL_PF_8888, emuCurrentPalettePixelFormat, ((u32*)cbp)[i]);
        }
    }

    // Update texture if it uses a palette format
    if (emuCurrentTexturePixelFormat == GU_PSM_T4 || emuCurrentTexturePixelFormat == GU_PSM_T8) {
        sceGuTexImage(0, emuCurrentTexSizeX, emuCurrentTexSizeY, emuCurrentTexSizeX, emuCurrentTextureSourceData);
    }
}

// Function to set CLUT mode
void sceGuClutMode(unsigned int cpsm, unsigned int a1, unsigned int a2, unsigned int a3) {
    // TODO: Implement full functionality
    emuCurrentPalettePixelFormat = cpsm;
}

// Function to start drawing
void sceGuStart(int cid, void* list) {
    if (!emuSkipFrame) {
        emuStartDrawing();
    }
}

// Function to synchronize rendering
int sceGuSync(int mode, int a1) {
    if (!emuSkipFrame) {
        glFinish();
    }
    return 0;
}

// Function to finish rendering
int sceGuFinish(void) {
    if (!emuSkipFrame) {
        emuEndDrawing();
    }
    return 0;
}

// Function to wait for V-Blank (Vertical Blank Interval)
void sceDisplayWaitVblankStart() {
    struct timeval timer;
    int last = 0, val;

    if (osl_curBuf == OSL_SECONDARY_BUFFER || !emuDoubleBuffer) {
        glFinish();
    }

    // Synchronize at 16.666 milliseconds (60 FPS)
    while (1) {
        gettimeofday(&timer, 0);
        val = (timer.tv_sec * 1000000 + timer.tv_usec) % 16666;
        if (val < last) {
            break;
        }
        last = val;
    }

    // Update vertical blank count
    if (emuFrameSkipMode == 0 && !emuPauseGame) {
        osl_vblCount = (timer.tv_sec * 1000000 + timer.tv_usec) / 16666;
    } else {
        osl_vblCount++;
    }

    // Handle game pause logic
    if (emuPauseGame == 2) {
        emuPauseGame = 1;
    }

    // Pause loop
    while (emuPauseGame == 1) {
        Sleep(10);
    }
}

// Function to display or hide the display (not yet implemented)
int sceGuDisplay(int state) {
    // TODO: Implement display state handling
    return 1;
}

// Function to set shading model
void sceGuShadeModel(int mode) {
    glShadeModel(mode);
}

// Helper function to set texture filters
void sceGuTexFilter(int min, int mag) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
}

// Set scissor box for rendering (coordinate system differs between PSP and OpenGL)
void sceGuScissor(int x, int y, int w, int h) {
    // Coordinates are inverted in OpenGL (0,0 is bottom-left)
    glScissor(x, 272 - y - oslAbs(h - y), oslAbs(w - x), oslAbs(h - y));
}

// Not supported draw buffer function
void sceGuDrawBuffer(int psm, void* fbp, int fbw) {
    // NOT SUPPORTED - Add proper handling if needed
}

// Initialize graphics (to be implemented)
void sceGuInit(void) {
    // TODO: Implement this function
}

// Debug screen initialization (currently empty)
void pspDebugScreenInit() {
    // No current implementation
}

// Swap buffers in double buffering mode
void* sceGuSwapBuffers(void) {
    if (emuDoubleBuffer) {
        SwapBuffers(wglGetCurrentDC()); // Swap the current OpenGL buffers
    }
    // If double buffering is not enabled, nothing is done
    return NULL;
}

// Clear screen with a specific color
void sceGuClearColor(unsigned int color) {
    glClearColor(
        (GLclampf)((color & 0xff) / 255.0),             // Red component
        (GLclampf)(((color >> 8) & 0xff) / 255.0),      // Green component
        (GLclampf)(((color >> 16) & 0xff) / 255.0),     // Blue component
        (GLclampf)(255)                                 // Full alpha
    );
    emuCurrentClearColor = color; // Store current clear color in emulator state
}

// Clear screen buffers based on flags
void sceGuClear(int flags) {
    glClear(flags);
}

// Terminate graphics context (to be implemented)
void sceGuTerm() {
    // TODO: Implement this function
}

// Exit game function for emulator
void sceKernelExitGame() {
    emuExit(); // Call the emulator exit function
}

// File I/O functions (PSP compatibility)
SceUID sceIoOpen(const char *file, int flags, SceMode mode) {
    FILE *f = NULL;
    // Create file if requested
    if (flags & PSP_O_CREAT) {
        fclose(fopen(file, "wb")); // Create file in write mode
    }

    // Open file based on requested flags
    if ((flags & 0xff) == PSP_O_RDONLY) {
        f = fopen(file, "rb");
    } else if ((flags & 0xff) == PSP_O_WRONLY) {
        f = fopen(file, "wb");
    } else if ((flags & 0xff) == PSP_O_RDWR) {
        f = fopen(file, "a+");
    }

    // Return an invalid file handle if opening failed
    if (!f) {
        return (SceUID)(-1);
    }

    return (SceUID)f;
}

// Close a file
int sceIoClose(SceUID fd) {
    return fclose((FILE*)fd);
}

// Read data from a file
int sceIoRead(SceUID fd, void *data, SceSize size) {
    return fread(data, 1, size, (FILE*)fd);
}

// Write data to a file
int sceIoWrite(SceUID fd, const void *data, SceSize size) {
    return fwrite(data, 1, size, (FILE*)fd);
}

// Set sampling cycle for controller (stub implementation)
int sceCtrlSetSamplingCycle(int cycle) {
    // Ok to return success for stub
    return 0;
}

// Set sampling mode for controller (stub implementation)
int sceCtrlSetSamplingMode(int mode) {
    // Ok to return success for stub
    return 0;
}

// Function to simulate reading PSP control data
int sceCtrlPeekBufferPositive(SceCtrlData *pad_data, int count) {
    if (!pad_data || count <= 0) {
        return 0; // Handle invalid input
    }

    pad_data->Buttons = 0;
    pad_data->Lx = 128; // Centered joystick
    pad_data->Ly = 128; // Centered joystick

    // Button mappings
    if (emuKeyHeld[VK_UP])
        pad_data->Buttons |= PSP_CTRL_UP;
    if (emuKeyHeld[VK_DOWN])
        pad_data->Buttons |= PSP_CTRL_DOWN;
    if (emuKeyHeld[VK_LEFT])
        pad_data->Buttons |= PSP_CTRL_LEFT;
    if (emuKeyHeld[VK_RIGHT])
        pad_data->Buttons |= PSP_CTRL_RIGHT;

    if (emuKeyHeld[VK_NUMPAD1] || emuKeyHeld['1'])
        pad_data->Buttons |= PSP_CTRL_SQUARE;
    if (emuKeyHeld[VK_NUMPAD2] || emuKeyHeld['2'])
        pad_data->Buttons |= PSP_CTRL_CROSS;
    if (emuKeyHeld[VK_NUMPAD3] || emuKeyHeld['3'])
        pad_data->Buttons |= PSP_CTRL_CIRCLE;
    if (emuKeyHeld[VK_NUMPAD5] || emuKeyHeld['5'])
        pad_data->Buttons |= PSP_CTRL_TRIANGLE;

    if (emuKeyHeld[VK_NUMPAD4] || emuKeyHeld['4'])
        pad_data->Buttons |= PSP_CTRL_LTRIGGER;
    if (emuKeyHeld[VK_NUMPAD6] || emuKeyHeld['6'])
        pad_data->Buttons |= PSP_CTRL_RTRIGGER;

    if (emuKeyHeld[VK_NUMPAD0] || emuKeyHeld['0'])
        pad_data->Buttons |= PSP_CTRL_SELECT;
    if (emuKeyHeld[VK_RETURN])
        pad_data->Buttons |= PSP_CTRL_START;

    // Joystick handling
    if (emuKeyHeld['A'])
        pad_data->Lx = 0;   // Move joystick to the left
    if (emuKeyHeld['D'])
        pad_data->Lx = 255; // Move joystick to the right
    if (emuKeyHeld['W'])
        pad_data->Ly = 0;   // Move joystick up
    if (emuKeyHeld['S'])
        pad_data->Ly = 255; // Move joystick down

    return 1;
}

// Read pixels in correct order (top <-> bottom inversion)
void emuGlReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) {
    if (!pixels || width <= 0 || height <= 0) {
        return; // Handle invalid input
    }

    // Avoid reading if skipping frame
    if (emuSkipFrame) {
        return;
    }

    int lineSize = (osl_pixelWidth[type] * width) >> 3; // Calculate line size in bytes

    // Read pixel data from OpenGL
    glReadPixels(x, y, width, height, format, emu_pixelPhysFormats[type], emuTempTextureMemory);

    // Flip the image vertically (invert top and bottom)
    for (int i = 0, j = height - 1; i < height; ++i, --j) {
        memcpy((u8*)pixels + i * lineSize, (u8*)emuTempTextureMemory + j * lineSize, lineSize);
    }
}

// Function to get current time
void gettimeofday(struct timeval *tv, int unused) {
    s64 currentValue;
    if (QueryPerformanceCounter((LARGE_INTEGER*)&currentValue)) {
        tv->tv_sec = (currentValue - emuCounterBaseValue) / emuCounterFrequency;
        tv->tv_usec = (((currentValue - emuCounterBaseValue) * 1000000) / emuCounterFrequency) % 1000000;
    } else {
        // Handle error case for QueryPerformanceCounter
        tv->tv_sec = tv->tv_usec = 0;
    }
}

// Configure 2D transfer
void emuConfigure2DTransfer(int enable) {
    static int bTextureEnabled = 0, bBlendingEnabled = 0;
    if (enable) {
        bTextureEnabled = glIsEnabled(GL_TEXTURE_2D);
        bBlendingEnabled = glIsEnabled(GL_BLEND);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        glColorMask(TRUE, TRUE, TRUE, TRUE);
    } else {
        if (bTextureEnabled) {
            glEnable(GL_TEXTURE_2D);
        }
        if (bBlendingEnabled) {
            glEnable(GL_BLEND);
        }
        glColorMask(TRUE, TRUE, TRUE, emuIsAlphaWriteEnabled);
    }
}

// Seek in file descriptor (PSP compatibility)
int sceIoLseek32(SceUID fd, int offset, int whence) {
    FILE* file = (FILE*)fd;
    int retval = ftell(file);
    if (fseek(file, offset, whence) != 0) {
        // Handle seek error
        return -1;
    }
    return retval;
}

// Audio functions - Not yet implemented
int sceAudioChReserve(int channel, int samplecount, int format) {
    return 0; // Not implemented
}

int sceAudioChRelease(int channel) {
    return 0; // Not implemented
}

int sceAudioOutputPannedBlocking(int channel, int leftvol, int rightvol, void *buffer) {
    return 0; // Not implemented
}

int sceAudioChangeChannelConfig(int channel, int format) {
    return 0; // Not implemented
}

// Thread management - Not yet implemented
SceUID sceKernelCreateThread(const char *name, SceKernelThreadEntry entry, int initPriority, int stackSize, SceUInt attr, SceKernelThreadOptParam *option) {
    return 0; // Not implemented
}

int sceKernelStartThread(SceUID thid, SceSize arglen, void *argp) {
    return 0; // Not implemented
}

int sceKernelWaitThreadEnd(SceUID thid, SceUInt *timeout) {
    return 0; // Not implemented
}

int sceKernelDeleteThread(SceUID thid) {
    return 0; // Not implemented
}

int sceKernelExitDeleteThread(int status) {
    return 0; // Not implemented
}

// Display buffer handling
void sceGuDispBuffer(int width, int height, void* dispbp, int dispbw) {
    if (dispbp) {
        emuDoubleBuffer = 1;
    } else {
        emuDoubleBuffer = 0;
        glReadBuffer(GL_FRONT);
        glDrawBuffer(GL_FRONT);
    }
}

// Texture wrapping
void sceGuTexWrap(int u, int v) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (u == GU_CLAMP) ? GL_CLAMP : GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (v == GU_CLAMP) ? GL_CLAMP : GL_REPEAT);
}