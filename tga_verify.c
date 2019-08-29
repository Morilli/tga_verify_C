#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>


int main(int argc, char** argv)
{
    if (argc != 2 && argc != 3) {
        printf("tga_verify will verify all tga files in the meta folder to ensure they will be read correctly by the WiiU.\n\n");
        printf("Usage: ./tga_verify [--fixup] <folder>\n\n");
        printf("If --fixup is given, tga_verify will attempt to fix all incorrect tgas. Contents may be overwritten.\n");
        exit(0);
    }
    bool fixup = false;
    if (argc == 3) {
        if (strcmp(argv[1], "--fixup") && strcmp(argv[1], "-f")) {
            fprintf(stderr, "Error: Unrecognized arguments.\n");
            exit(EXIT_FAILURE);
        }

        fixup = true;
    }

    const char tgas[4][16] = {"iconTex.tga", "bootLogoTex.tga", "bootDrcTex.tga", "bootTvTex.tga", };
    const uint16_t dimensions[4][3] = { {128,128,32}, {170,42,32}, {854,480,24}, {1280,720,24} };
    const char dimension_name[3][7] = {"width", "height", "depth"};

    int foldername_length = strlen(argv[argc-1]);
    char filename[foldername_length + 18];
    strcpy(filename, argv[argc-1]);
    if (filename[foldername_length-1] != '/' && filename[foldername_length-1] != '\\') {
        filename[foldername_length] = '/';
        filename[foldername_length+1] = '\0';
        foldername_length++;
    }

    for (int i = 0; i < 4; i++) {
        strcpy(&filename[foldername_length], tgas[i]);
        FILE* tga_file = fixup ? fopen(filename, "rb+") : fopen(filename, "rb");
        if (!tga_file) {
            fprintf(stderr, "Error: The file \"%s\" can't be accessed. Make sure it exists and is accessable.\n", filename);
            continue;
        }

        if (fixup) {
            fwrite("\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00", 1, 12, tga_file);
            fwrite(dimensions[i], 2, 1, tga_file);
            fwrite(&dimensions[i][1], 2, 1, tga_file);
            fwrite(&dimensions[i][2], 1, 1, tga_file);
            fseek(tga_file, 1 + dimensions[i][0] * dimensions[i][1] * dimensions[i][2]/8, SEEK_CUR);
            fwrite("\x00\x00\x00\x00\x00\x00\x00\x00TRUEVISION-XFILE.", 1, 26, tga_file);
        } else {
            uint8_t current_header[12];
            if (fread(current_header, 1, 12, tga_file) != 12) {
                fprintf(stderr, "Error: The file \"%s\" is not long enough.\n", filename);
                goto continue_point;
            }
            if (memcmp(current_header, "\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00", 12) != 0) {
                fprintf(stderr, "Error in header of file \"%s\", make sure the file is uncompressed.\n", filename);
                goto continue_point;
            }

            uint16_t current_dimensions[3] = {0};
            if (fread(current_dimensions, 2, 1, tga_file) != 1 || fread(&current_dimensions[1], 2, 1, tga_file) != 1 || fread(&current_dimensions[2], 1, 1, tga_file) != 1) {
                fprintf(stderr, "Error: The file \"%s\" is not long enough.\n", filename);
                goto continue_point;
            }
            bool dimensions_mismatch = false;
            for (int j = 0; j < 3; j++) {
                if (current_dimensions[j] != dimensions[i][j]) {
                    fprintf(stderr, "Error: The file \"%s\" has a %s of %d; expected %d.\n", filename, dimension_name[j], current_dimensions[j], dimensions[i][j]);
                    dimensions_mismatch = true;
                }
            }
            if (dimensions_mismatch)
                goto continue_point;

            fseek(tga_file, 1 + current_dimensions[0] * current_dimensions[1] * current_dimensions[2]/8, SEEK_CUR);
            uint8_t truevision[26];
            if (fread(truevision, 1, 26, tga_file) != 26) {
                fprintf(stderr, "Error: The file \"%s\" is not long enough. The \"TRUEVISION\" footer may be missing.\n", filename);
                goto continue_point;
            }
            if (memcmp(truevision, "\x00\x00\x00\x00\x00\x00\x00\x00TRUEVISION-XFILE.", 26) != 0) {
                fprintf(stderr, "Error: Truevision footer incorrect for file \"%s\".\n", filename);
                goto continue_point;
            }
        }

        printf("Successfully verified \"%s\".\n", filename);
        continue_point:
        fclose(tga_file);
    }
}
