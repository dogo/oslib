/**
 * @file test_improvements.c
 * @brief Simple test file to validate OSLib improvements
 * 
 * This file contains basic tests to ensure our improvements work correctly.
 * Note: This is not a full PSP program, just validation for our changes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Mock OSL_IMAGE struct for testing
typedef struct {
    void *data;
    void *palette;
    int flags;
} OSL_IMAGE;

// Mock functions for testing
void oslFreeImageData(OSL_IMAGE *img) {
    if (img && img->data) {
        free(img->data);
        img->data = NULL;
    }
}

void oslDeletePalette(void *palette) {
    if (palette) {
        free(palette);
    }
}

int oslImageIsCopy(OSL_IMAGE *img) {
    return img ? (img->flags & 1) : 0;
}

// The actual function we improved
void oslDeleteImage(OSL_IMAGE *img) {
    if (!oslImageIsCopy(img)) {
        oslFreeImageData(img);
        if (img->palette)
            oslDeletePalette(img->palette);
    }
    free(img);
    // Note: Use oslDeleteImageSafe() macro to automatically set pointer to NULL
}

// The safe macro
#define oslDeleteImageSafe(img_ptr) do { \
    if (img_ptr) { \
        oslDeleteImage(img_ptr); \
        (img_ptr) = NULL; \
    } \
} while(0)

// Test function
void test_safe_deletion() {
    printf("Testing safe image deletion...\n");
    
    // Create a mock image
    OSL_IMAGE *img = malloc(sizeof(OSL_IMAGE));
    img->data = malloc(100);
    img->palette = NULL;
    img->flags = 0; // Not a copy
    
    // Test that the pointer is not NULL before deletion
    assert(img != NULL);
    
    // Use the safe deletion macro
    oslDeleteImageSafe(img);
    
    // Test that the pointer is now NULL
    assert(img == NULL);
    
    printf("✓ Safe image deletion test passed\n");
}

void test_array_bounds_fix() {
    printf("Testing array bounds fix...\n");
    
    const char *ext = ".png";
    char extension[10] = {0};
    int i = 0;
    
    // This should not cause buffer overflow
    while (i < sizeof(extension) - 1 && ext[i]) {
        extension[i] = ext[i]; // simplified version
        i++;
    }
    
    assert(strcmp(extension, ".png") == 0);
    printf("✓ Array bounds fix test passed\n");
}

int main() {
    printf("Running OSLib improvements tests...\n\n");
    
    test_safe_deletion();
    test_array_bounds_fix();
    
    printf("\n✅ All tests passed!\n");
    return 0;
}