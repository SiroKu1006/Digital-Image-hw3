#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#pragma warning(disable:4996)

typedef long INT32;
typedef unsigned short int INT16;
typedef unsigned char U_CHAR;

#define UCH(x)  ((int) (x))
#define GET_2B(array,offset)  ((INT16) UCH(array[offset]) + (((INT16) UCH(array[offset+1])) << 8))
#define GET_4B(array,offset)  ((INT32) UCH(array[offset]) + (((INT32) UCH(array[offset+1])) << 8) + (((INT32) UCH(array[offset+2])) << 16) + (((INT32) UCH(array[offset+3])) << 24))
#define FREAD(file,buf,sizeofbuf)  ((size_t) fread((void *) (buf), (size_t) 1, (size_t) (sizeofbuf), (file)))

int kernel1[3][3] = {
    {0, -1, 0},
    {-1, 4, -1},
    {0, -1, 0}
};

int kernel2[3][3] = {
    {-1, -1, -1},
    {-1, 8, -1},
    {-1, -1, -1}
};

void apply_laplacian(unsigned char* src, unsigned char* dst, int width, int height, int row_size, int kernel[3][3], int c, int add_original) {
    int i, j, m, n;
    for (i = 1; i < height - 1; i++) {
        for (j = 1; j < width - 1; j++) {
            int sum = 0;
            for (m = -1; m <= 1; m++) {
                for (n = -1; n <= 1; n++) {
                    int pixel = src[(i + m) * row_size + (j + n)];
                    sum += kernel[m + 1][n + 1] * pixel;
                }
            }
            int result = add_original ? src[i * row_size + j] + c * sum : sum;
            if (result < 0) result = 0;
            if (result > 255) result = 255;
            dst[i * row_size + j] = (unsigned char)result;
        }
    }
}

int main() {
    FILE *input_file = 0, *output_file = 0;
    U_CHAR bmpfileheader[14] = { 0 }, bmpinfoheader[40] = { 0 }, color_table[1024];
    INT32 FileSize, bfOffBits, headerSize, biWidth, biHeight;
    INT16 biPlanes, BitCount;

    if ((input_file = fopen("Fig0338(a).bmp", "rb")) == NULL) {
        fprintf(stderr, "File can't open.\n");
        exit(0);
    }

    FREAD(input_file, bmpfileheader, 14);
    FREAD(input_file, bmpinfoheader, 40);

    if (GET_2B(bmpfileheader, 0) != 0x4D42) {
        fprintf(stdout, "Not bmp file.\n");
        exit(0);
    }

    FileSize = GET_4B(bmpfileheader, 2);
    bfOffBits = GET_4B(bmpfileheader, 10);
    headerSize = GET_4B(bmpinfoheader, 0);
    biWidth = GET_4B(bmpinfoheader, 4);
    biHeight = GET_4B(bmpinfoheader, 8);
    biPlanes = GET_2B(bmpinfoheader, 12);
    BitCount = GET_2B(bmpinfoheader, 14);

    if (BitCount != 8) {
        fprintf(stderr, "Not a 8-bit file.\n");
        fclose(input_file);
        exit(0);
    }

    FREAD(input_file, color_table, 1024);

    int row_size = ((biWidth + 3) / 4) * 4;
    int img_size = row_size * biHeight;

    unsigned char* data = (unsigned char*)malloc(img_size);
    unsigned char* output1 = (unsigned char*)calloc(1, img_size);
    unsigned char* output2 = (unsigned char*)calloc(1, img_size);
    unsigned char* output3 = (unsigned char*)calloc(1, img_size);

    fseek(input_file, bfOffBits, SEEK_SET);
    FREAD(input_file, data, img_size);
    fclose(input_file);

    apply_laplacian(data, output1, biWidth, biHeight, row_size, kernel1, 0, 0);
    apply_laplacian(data, output2, biWidth, biHeight, row_size, kernel1, -1, 1);
    apply_laplacian(data, output3, biWidth, biHeight, row_size, kernel2, -1, 1);

    const char* filenames[] = {"laplacian.bmp", "sharpened_c-1.bmp", "sharpened_alt_kernel.bmp"};
    unsigned char* outputs[] = {output1, output2, output3};

    for (int f = 0; f < 3; f++) {
        output_file = fopen(filenames[f], "wb");
        fwrite(bmpfileheader, sizeof(bmpfileheader), 1, output_file);
        fwrite(bmpinfoheader, sizeof(bmpinfoheader), 1, output_file);
        fwrite(color_table, 1024, 1, output_file);
        fwrite(outputs[f], img_size, 1, output_file);
        fclose(output_file);
    }

    free(data);
    free(output1);
    free(output2);
    free(output3);
    return 0;
}
