#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: ./tga_verify <folder>\n");
        exit(0);
    }

    const char tgas[4][16] = {"iconTex.tga", "bootLogoTex.tga", "bootDrcTex.tga", "bootTvTex.tga", };
    const uint16_t dimensions[4][3] = { {128,128,32}, {170,42,32}, {854,480,24}, {1280,720,24} };
    const uint8_t header[12] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const char dimension_name[3][7] = {"width", "height", "depth"};

    int foldername_length = strlen(argv[1]);
    char filename[foldername_length + 18];
    strcpy(filename, argv[1]);
    if (filename[foldername_length-1] != '/' && filename[foldername_length-1] != '\\') {
        filename[foldername_length] = '/';
        filename[foldername_length+1] = '\0';
        foldername_length++;
    }

    for (int i = 0; i < 4; i++) {
        strcpy(&filename[foldername_length], tgas[i]);
        FILE *tga_file = fopen(filename, "rb");
        if (!tga_file) {
            fprintf(stderr, "Error: file \"%s\" can't be accessed. Make sure it exists and is accessable.\n", filename);
            continue;
        }

        uint8_t current_header[12];
        if (fread(current_header, 1, 12, tga_file) != 12) {
            fprintf(stderr, "Error: The file \"%s\" is not long enough.\n", filename);
            continue;
        }
        if (memcmp(current_header, header, 12) != 0) {
            fprintf(stderr, "Error in header of file \"%s\", make sure the file is uncompressed.\n", filename);
            continue;
        }

        uint16_t current_dimensions[3] = {0};
        if (fread(current_dimensions, 2, 1, tga_file) != 1 || fread(&current_dimensions[1], 2, 1, tga_file) != 1 || fread(&current_dimensions[2], 1, 1, tga_file) != 1) {
            fprintf(stderr, "Error: The file \"%s\" is not long enough.\n", filename);
            continue;
        }
        for (int j = 0; j < 3; j++) {
            if (current_dimensions[j] != dimensions[i][j]) {
                fprintf(stderr, "Error: \"%s\" has a %s of %d; expected %d.\n", filename, dimension_name[j], current_dimensions[j], dimensions[i][j]);
            }
        }

        fseek(tga_file, 1 + current_dimensions[0] * current_dimensions[1] * current_dimensions[2]/8, SEEK_CUR);
        uint8_t truevision[26];
        if (fread(truevision, 1, 26, tga_file) != 26) {
            fprintf(stderr, "Error: The file \"%s\" is not long enough.\n", filename);
            continue;
        }
        if (memcmp(truevision, "\x00\x00\x00\x00\x00\x00\x00\x00TRUEVISION-XFILE.", 26) != 0) {
            fprintf(stderr, "Error: Truevision footer incorrect for file \"%s\".\n", filename);
            continue;
        }

        fclose(tga_file);
        printf("Successfully verified \"%s\".\n", filename);
    }
}
