/**
 * Kara Pelster
 * CISC372
 * HW6
 * PTHREADS
 **/

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include "image_PT.h"
#include <pthread.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// N is number of threads... so you will always have 10 threads unless you change the number here
#define N 10

// unlike serial version and open MP, thread_count, Images, and type is global so that we can use them in starter function for pthread
int thread_count;
Image srcImage, destImage, bwImage;
enum KernelTypes type;

//An array of kernel matrices to be used for image convolution.
//The indexes of these match the enumeration from the header file. ie. algorithms[BLUR] returns the kernel corresponding to a box blur.
Matrix algorithms[] = {
    {{0, -1, 0}, {-1, 4, -1}, {0, -1, 0}},
    {{0, -1, 0}, {-1, 5, -1}, {0, -1, 0}},
    {{1 / 9.0, 1 / 9.0, 1 / 9.0}, {1 / 9.0, 1 / 9.0, 1 / 9.0}, {1 / 9.0, 1 / 9.0, 1 / 9.0}},
    {{1.0 / 16, 1.0 / 8, 1.0 / 16}, {1.0 / 8, 1.0 / 4, 1.0 / 8}, {1.0 / 16, 1.0 / 8, 1.0 / 16}},
    {{-2, -1, 0}, {-1, 1, 1}, {0, 1, 2}},
    {{0, 0, 0}, {0, 1, 0}, {0, 0, 0}}};

//getPixelValue - Computes the value of a specific pixel on a specific channel using the selected convolution kernel
//Paramters: srcImage:  An Image struct populated with the image being convoluted
//           x: The x coordinate of the pixel
//          y: The y coordinate of the pixel
//          bit: The color channel being manipulated
//          algorithm: The 3x3 kernel matrix to use for the convolution
//Returns: The new value for this x,y pixel and bit channel
uint8_t getPixelValue(Image *srcImage, int x, int y, int bit, Matrix algorithm)
{
    int px, mx, py, my, i, span;
    span = srcImage->width * srcImage->bpp;
    // for the edge pixes, just reuse the edge pixel
    px = x + 1;
    py = y + 1;
    mx = x - 1;
    my = y - 1;
    if (mx < 0)
        mx = 0;
    if (my < 0)
        my = 0;
    if (px >= srcImage->width)
        px = srcImage->width - 1;
    if (py >= srcImage->height)
        py = srcImage->height - 1;
    uint8_t result =
        algorithm[0][0] * srcImage->data[Index(mx, my, srcImage->width, bit, srcImage->bpp)] +
        algorithm[0][1] * srcImage->data[Index(x, my, srcImage->width, bit, srcImage->bpp)] +
        algorithm[0][2] * srcImage->data[Index(px, my, srcImage->width, bit, srcImage->bpp)] +
        algorithm[1][0] * srcImage->data[Index(mx, y, srcImage->width, bit, srcImage->bpp)] +
        algorithm[1][1] * srcImage->data[Index(x, y, srcImage->width, bit, srcImage->bpp)] +
        algorithm[1][2] * srcImage->data[Index(px, y, srcImage->width, bit, srcImage->bpp)] +
        algorithm[2][0] * srcImage->data[Index(mx, py, srcImage->width, bit, srcImage->bpp)] +
        algorithm[2][1] * srcImage->data[Index(x, py, srcImage->width, bit, srcImage->bpp)] +
        algorithm[2][2] * srcImage->data[Index(px, py, srcImage->width, bit, srcImage->bpp)];
    return result;
}

//convolute:  Applies a kernel matrix to an image
//Parameters: srcImage: The image being convoluted
//            destImage: A pointer to a  pre-allocated (including space for the pixel array) structure to receive the convoluted image.  It should be the same size as srcImage
//            algorithm: The kernel matrix to use for the convolution
//Returns: Nothing
void convolute(Image *srcImage, Image *destImage, Matrix algorithm, long my_rank)
{
    // printf("begin convolve %d\n", my_rank);

    int local_height = ((srcImage->height + thread_count - 1) / thread_count);

    int local_min = (my_rank) * local_height;
    int local_max = (my_rank + 1) * local_height;

    // printf("local min: %d local max: %d\n", local_min, local_max);

    int row, pix, bit, span;
    span = srcImage->bpp * srcImage->bpp;
    // for (row=0;row<srcImage->height;row++){
    for (row = local_min; row < local_max && row < srcImage->height; row++)
    {
        for (pix = 0; pix < srcImage->width; pix++)
        {
            for (bit = 0; bit < srcImage->bpp; bit++)
            {
                destImage->data[Index(pix, row, srcImage->width, bit, srcImage->bpp)] = getPixelValue(srcImage, pix, row, bit, algorithm);
            }
        }
    }
}

//convoluve:  start method for each pthread. calls convolute using globally defined variables srcImage, destImage, algorithms[type], my_rank
//Parameters: rank: void type rank of pthread
//Returns: Nothing
void *convolve(void *rank)
{

    long my_rank = (long)rank;
    // printf("here in convolve\n");
    convolute(&srcImage, &destImage, algorithms[type], my_rank);
}

//Usage: Prints usage information for the program
//Returns: -1
int Usage(char *executable)
{
    printf("Usage: %s <filename> <type>\n\twhere type is one of (edge,sharpen,blur,gauss,emboss,identity)\n", executable);
    return -1;
}

//GetKernelType: Converts the string name of a convolution into a value from the KernelTypes enumeration
//Parameters: type: A string representation of the type
//Returns: an appropriate entry from the KernelTypes enumeration, defaults to IDENTITY, which does nothing but copy the image.
enum KernelTypes GetKernelType(char *type)
{
    if (!strcmp(type, "edge"))
        return EDGE;
    else if (!strcmp(type, "sharpen"))
        return SHARPEN;
    else if (!strcmp(type, "blur"))
        return BLUR;
    else if (!strcmp(type, "gauss"))
        return GAUSE_BLUR;
    else if (!strcmp(type, "emboss"))
        return EMBOSS;
    else
        return IDENTITY;
}

//main:
//argv is expected to take 2 arguments.  First is the source file name (can be jpg, png, bmp, tga).  Second is the lower case name of the algorithm.
int main(int argc, char **argv)
{
    printf("running using pthreads...\n\n");

    long t1, t2;
    t1 = time(NULL);

    stbi_set_flip_vertically_on_load(0);
    if (argc != 3)
        return Usage(argv[0]);
    char *fileName = argv[1];
    // printf("arg1: %s\narg2: %s\n", argv[1], argv[2]);
    // printf("argv1: %d\nargv2: %d\n", strcmp(argv[1], "pic4.jpg"), strcmp(argv[2], "gauss"));
    if (!strcmp(argv[1], "pic4.jpg") && !strcmp(argv[2], "gauss"))
    {
        printf("You have applied a gaussian filter to Gauss which has caused a tear in the time-space continum.\n");
    }

    type = GetKernelType(argv[2]);

    srcImage.data = stbi_load(fileName, &srcImage.width, &srcImage.height, &srcImage.bpp, 0);

    if (!srcImage.data)
    {
        printf("Error loading file %s.\n", fileName);
        return -1;
    }

    destImage.bpp = srcImage.bpp;
    destImage.height = srcImage.height;
    destImage.width = srcImage.width;
    destImage.data = malloc(sizeof(uint8_t) * destImage.width * destImage.bpp * destImage.height);

    // begin thread
    long thread;
    pthread_t *thread_handles;

    thread_count = N;
    thread_handles = (pthread_t *)malloc(thread_count * sizeof(pthread_t));

    // printf("tc: %d\n", thread_count);

    for (thread = 0; thread < thread_count; thread++)
    {
        // printf("in thread loop %d\n", thread);
        pthread_create(&thread_handles[thread], NULL, &convolve, (void *)thread);
    }

    for (thread = 0; thread < thread_count; thread++)
    {
        pthread_join(thread_handles[thread], NULL);
    }

    // printf("before threadhandles free\n");
    free(thread_handles);
    // printf("after threadhandles free\n");

    // printf("here");

    stbi_write_png("output.png", destImage.width, destImage.height, destImage.bpp, destImage.data, destImage.bpp * destImage.width);
    stbi_image_free(srcImage.data);

    free(destImage.data);
    t2 = time(NULL);
    printf("Took %ld seconds\n", t2 - t1);
    return 0;
}