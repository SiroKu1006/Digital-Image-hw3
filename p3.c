#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#pragma warning(disable:4996)

#define UCH(x)  ((int) (x))
typedef unsigned char U_CHAR;

typedef long INT32;
typedef unsigned short int INT16;

#define GET_2B(array,offset)  ((INT16) UCH(array[offset]) + (((INT16) UCH(array[offset+1])) << 8))
#define GET_4B(array,offset)  ((INT32) UCH(array[offset]) + (((INT32) UCH(array[offset+1])) << 8) + (((INT32) UCH(array[offset+2])) << 16) + (((INT32) UCH(array[offset+3])) << 24))
#define FREAD(file,buf,sizeofbuf)  ((size_t) fread((void *) (buf), (size_t) 1, (size_t) (sizeofbuf), (file)))

int sobel_x[3][3] = { {-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1} };
int sobel_y[3][3] = { {-1, -2, -1}, {0, 0, 0}, {1, 2, 1} };
int laplacian[3][3] = { {0, -1, 0}, {-1, 4, -1}, {0, -1, 0} };

void apply_filter(unsigned char* src, unsigned char* dst, int width, int height, int row_size, int kernel[3][3]) {
    for (int i = 1; i < height - 1; i++) {
        for (int j = 1; j < width - 1; j++) {
            int sum = 0;
            for (int m = -1; m <= 1; m++) {
                for (int n = -1; n <= 1; n++) {
                    int pixel = src[(i + m) * row_size + (j + n)];
                    sum += kernel[m + 1][n + 1] * pixel;
                }
            }
            int result = sum;
            if (result < 0) result = 0;
            if (result > 255) result = 255;
            dst[i * row_size + j] = (unsigned char)result;
        }
    }
}

void apply_sobel(unsigned char* src, unsigned char* dst, int width, int height, int row_size) {
    for (int i = 1; i < height - 1; i++) {
        for (int j = 1; j < width - 1; j++) {
            int gx = 0, gy = 0;
            for (int m = -1; m <= 1; m++) {
                for (int n = -1; n <= 1; n++) {
                    int pixel = src[(i + m) * row_size + (j + n)];
                    gx += sobel_x[m + 1][n + 1] * pixel;
                    gy += sobel_y[m + 1][n + 1] * pixel;
                }
            }
            int mag = (int)sqrt(gx * gx + gy * gy);
            if (mag > 255) mag = 255;
            dst[i * row_size + j] = (unsigned char)mag;
        }
    }
}

void apply_average(unsigned char* src, unsigned char* dst, int width, int height, int row_size) {
    for (int i = 2; i < height - 2; i++) {
        for (int j = 2; j < width - 2; j++) {
            int sum = 0;
            for (int m = -2; m <= 2; m++) {
                for (int n = -2; n <= 2; n++) {
                    sum += src[(i + m) * row_size + (j + n)];
                }
            }
            dst[i * row_size + j] = sum / 25;
        }
    }
}

void image_add(unsigned char* a, unsigned char* b, unsigned char* dst, int size) {
    for (int i = 0; i < size; i++) {
        int sum = a[i] + b[i];
        dst[i] = (sum > 255) ? 255 : sum;
    }
}

void image_multiply(unsigned char* a, unsigned char* b, unsigned char* dst, int size) {
    for (int i = 0; i < size; i++) {
        int val = a[i] * b[i] / 255;
        dst[i] = (val > 255) ? 255 : val;
    }
}

void power_law(unsigned char* src, unsigned char* dst, int size, float gamma) {
    for (int i = 0; i < size; i++) {
        float norm = src[i] / 255.0f;
        float result = pow(norm, gamma) * 255.0f;
        dst[i] = (result > 255) ? 255 : (unsigned char)result;
    }
}

int main() {
    FILE *input_file = 0, *output_file = 0;
    U_CHAR bmpfileheader[14] = { 0 }, bmpinfoheader[40] = { 0 }, color_table[1024];
    INT32 biWidth, biHeight;
    INT16 BitCount;

    if ((input_file = fopen("Fig3.43(a).bmp", "rb")) == NULL) {
        fprintf(stderr, "File can't open.\n");
        exit(0);
    }

    FREAD(input_file, bmpfileheader, 14);
    FREAD(input_file, bmpinfoheader, 40);
    biWidth = GET_4B(bmpinfoheader, 4);
    biHeight = GET_4B(bmpinfoheader, 8);
    BitCount = GET_2B(bmpinfoheader, 14);

    if (BitCount != 8) {
        fprintf(stderr, "Not an 8-bit BMP file.\n");
        exit(0);
    }

    FREAD(input_file, color_table, 1024);
    int row_size = ((biWidth + 3) / 4) * 4;
    int img_size = row_size * biHeight;

    unsigned char *a = (unsigned char*)malloc(img_size); // 原圖 (a)
    unsigned char *b = (unsigned char*)calloc(1, img_size); // Laplacian (b)
    unsigned char *c = (unsigned char*)calloc(1, img_size); // a + b
    unsigned char *d = (unsigned char*)calloc(1, img_size); // Sobel (d)
    unsigned char *e = (unsigned char*)calloc(1, img_size); // 平滑(d)
    unsigned char *f = (unsigned char*)calloc(1, img_size); // c * e
    unsigned char *g = (unsigned char*)calloc(1, img_size); // a + f
    unsigned char *h = (unsigned char*)calloc(1, img_size); // gamma(g)

    fseek(input_file, GET_4B(bmpfileheader, 10), SEEK_SET);
    FREAD(input_file, a, img_size);
    fclose(input_file);

    apply_filter(a, b, biWidth, biHeight, row_size, laplacian);
    image_add(a, b, c, img_size);
    apply_sobel(a, d, biWidth, biHeight, row_size);
    apply_average(d, e, biWidth, biHeight, row_size);
    image_multiply(c, e, f, img_size);
    image_add(a, f, g, img_size);
    power_law(g, h, img_size, 0.5f);

    const char* names[] = {"a_original.bmp", "b_laplacian.bmp", "c_add_ab.bmp", "d_sobel.bmp", "e_smooth_d.bmp", "f_mult_ce.bmp", "g_add_af.bmp", "h_gamma_g.bmp"};
    unsigned char* results[] = {a, b, c, d, e, f, g, h};

    for (int i = 0; i < 8; i++) {
        output_file = fopen(names[i], "wb");
        fwrite(bmpfileheader, sizeof(bmpfileheader), 1, output_file);
        fwrite(bmpinfoheader, sizeof(bmpinfoheader), 1, output_file);
        fwrite(color_table, 1024, 1, output_file);
        fwrite(results[i], img_size, 1, output_file);
        fclose(output_file);
    }

    free(a); free(b); free(c); free(d); free(e); free(f); free(g); free(h);
    return 0;
}