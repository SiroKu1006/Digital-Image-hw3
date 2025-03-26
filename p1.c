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

int main()
{
    FILE *input_file = 0;
    FILE *output_file = 0;

    U_CHAR bmpfileheader[14] = { 0 };
    U_CHAR bmpinfoheader[40] = { 0 };

    INT32 FileSize = 0;
    INT32 bfOffBits = 0;
    INT32 headerSize = 0;
    INT32 biWidth = 0;
    INT32 biHeight = 0;
    INT16 biPlanes = 0;
    INT16 BitCount = 0;
    INT32 biCompression = 0;
    INT32 biImageSize = 0;
    INT32 biXPelsPerMeter = 0, biYPelsPerMeter = 0;
    INT32 biClrUsed = 0;
    INT32 biClrImp = 0;

    U_CHAR *data, *new_data, color_table[1024];
    int i, j, k;
    int histo_table[256] = { 0 };
    float cdf[256] = { 0 };
    float target_pdf[256] = { 0 };
    float target_cdf[256] = { 0 };
    unsigned char mapping[256];

    if ((input_file = fopen("Fig3.23(a).bmp", "rb")) == NULL) {
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
    biCompression = GET_4B(bmpinfoheader, 16);
    biImageSize = GET_4B(bmpinfoheader, 20);
    biXPelsPerMeter = GET_4B(bmpinfoheader, 24);
    biYPelsPerMeter = GET_4B(bmpinfoheader, 28);
    biClrUsed = GET_4B(bmpinfoheader, 32);
    biClrImp = GET_4B(bmpinfoheader, 36);

    if (BitCount != 8) {
        fprintf(stderr, "Not a 8-bit file.\n");
        fclose(input_file);
        exit(0);
    }

    FREAD(input_file, color_table, 1024);

    data = (U_CHAR *)malloc(((biWidth + 3) / 4 * 4)*biHeight);
    if (data == NULL) {
        fprintf(stderr, "Insufficient memory.\n");
        fclose(input_file);
        exit(0);
    }

    fseek(input_file, bfOffBits, SEEK_SET);
    FREAD(input_file, data, ((biWidth + 3) / 4 * 4)*biHeight);
    fclose(input_file);

    int row_size = ((biWidth + 3) / 4 * 4);
    int total_pixels = biWidth * biHeight;

    for (i = 0; i < biHeight; i++) {
        k = i * row_size;
        for (j = 0; j < biWidth; j++) {
            histo_table[data[k + j]]++;
        }
    }

    cdf[0] = (float)histo_table[0] / total_pixels;
    for (i = 1; i < 256; i++) {
        cdf[i] = cdf[i - 1] + (float)histo_table[i] / total_pixels;
    }

    float sum = 0;
    for (i = 0; i < 256; i++) {
        target_pdf[i] = exp(-pow((i - 64) / 20.0, 2)) + exp(-pow((i - 192) / 20.0, 2));
        sum += target_pdf[i];
    }
    for (i = 0; i < 256; i++) {
        target_pdf[i] /= sum;
    }

    target_cdf[0] = target_pdf[0];
    for (i = 1; i < 256; i++) {
        target_cdf[i] = target_cdf[i - 1] + target_pdf[i];
    }

    for (i = 0; i < 256; i++) {
        float min_diff = 1.0f;
        int best = 0;
        for (j = 0; j < 256; j++) {
            float diff = fabs(cdf[i] - target_cdf[j]);
            if (diff < min_diff) {
                min_diff = diff;
                best = j;
            }
        }
        mapping[i] = best;
    }

    for (i = 0; i < biHeight; i++) {
        k = i * row_size;
        for (j = 0; j < biWidth; j++) {
            data[k + j] = mapping[data[k + j]];
        }
    }

    if ((output_file = fopen("p1.bmp", "wb")) == NULL) {
        fprintf(stderr, "Output file can't open.\n");
        exit(0);
    }

    fwrite(bmpfileheader, sizeof(bmpfileheader), 1, output_file);
    fwrite(bmpinfoheader, sizeof(bmpinfoheader), 1, output_file);
    fwrite(color_table, 1024, 1, output_file);
    fwrite(data, ((biWidth + 3) / 4 * 4)*biHeight, 1, output_file);
    fclose(output_file);

    free(data);
    return 0;
}