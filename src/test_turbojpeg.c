#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>
 
// #include "jpeg-utils.h"
// #include "libjpeg/include/turbojpeg.h"
#include  <dirent.h>
#include <unistd.h>

#include <turbojpeg.h>

#define NUM_IMG 100


struct JpegHeader
{
    long int size;
    int width;
    int height;
};


char *imgArray[NUM_IMG];
struct JpegHeader imgHeaderArray[NUM_IMG];


 
int tjpeg_header(unsigned char* jpeg_buffer, int jpeg_size, int* width, int* height, int* subsample, int* colorspace)
{
    tjhandle handle = NULL;
 
    handle = tjInitDecompress();
    tjDecompressHeader3(handle, jpeg_buffer, jpeg_size, width, height, subsample, colorspace);
 
    tjDestroy(handle);
 
    return 0;
}
 
int tjpeg2rgb(unsigned char* jpeg_buffer, int jpeg_size, unsigned char* rgb_buffer, int arrayIndex)
{
    tjhandle handle = NULL;
    int width, height, subsample, colorspace;
    int flags = 0;
    int pixelfmt = TJPF_RGB;
 
    handle = tjInitDecompress();
    tjDecompressHeader3(handle, jpeg_buffer, jpeg_size, &width, &height, &subsample, &colorspace);

    imgHeaderArray[arrayIndex].size = jpeg_size;
    imgHeaderArray[arrayIndex].width = width;
    imgHeaderArray[arrayIndex].height = height;
 
    flags |= 0;
    tjDecompress2(handle, jpeg_buffer, jpeg_size, rgb_buffer, width, 0,
            height, pixelfmt, flags);
 
    tjDestroy(handle);
 
    return 0;
}
 
int trgb2jpeg(unsigned char* rgb_buffer, int width, int height, int quality, unsigned char** jpeg_buffer, unsigned long* jpeg_size)
{
    tjhandle handle = NULL;
    //unsigned long size=0;
    int flags = 0;
    int subsamp = TJSAMP_422;
    int pixelfmt = TJPF_RGB;
 
    handle=tjInitCompress();
    //size=tjBufSize(width, height, subsamp);
    tjCompress2(handle, rgb_buffer, width, 0, height, pixelfmt, jpeg_buffer, jpeg_size, subsamp,
            quality, flags);
 
    tjDestroy(handle);
 
    return 0;
}


void read_dir_img (char path[])
{
    DIR *dir;
    struct dirent *ptr;
    // unsigned count = 0;

    if ((dir=opendir(path)) == NULL)
    {
        perror("Open dir error...");
        exit(1);
    }

    // while ((ptr=readdir(dir)) != NULL)
    // {
    //     if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    ///current dir OR parrent dir
    //         continue;
    //     else if(ptr->d_type == 8)    ///file
    //     {
    //         // if ((jpegFile = fopen(argv[1], "rb")) == NULL)
    //         count += 1;
    //     }
    //     else 
    //     {
    //     continue;
    //     }
    // }

    if ((dir=opendir(path)) == NULL)
    {
        perror("Open dir error...");
        exit(1);
    }

    int i = -1;
    while ((ptr=readdir(dir)) != NULL, i < NUM_IMG)
    {
        i += 1;
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    ///current dir OR parrent dir
            continue;
        else if(ptr->d_type == 8)    ///file
        {
            FILE *imgFile = NULL;
            unsigned char *jpegBuf = NULL;
            int size = 0;
            if (fseek(imgFile, 0, SEEK_END) < 0 || ((size = ftell(imgFile)) < 0) ||
            fseek(imgFile, 0, SEEK_SET) < 0)
            {
                // printf ("File size error.\n");
            }
            if (size == 0)
            {
                // printf ("Size equals to zero");
            }
            unsigned long jpegSize = (unsigned long)size;
            // unsigned char *jpegBuf = NULL;
            if ((jpegBuf = (unsigned char *)tjAlloc(jpegSize)) == NULL)
            {
                // THROW_UNIX("allocating JPEG buffer");
            }
            if (fread(jpegBuf, jpegSize, 1, imgFile) < 1)
            {
                // THROW_UNIX("reading input file");
            }
            fclose(imgFile);  imgFile = NULL;
            tjpeg2rgb(jpegBuf, jpegSize, imgArray[i], i);
            
        }
        else 
        {
        continue;
        }
    }

    closedir(dir);
}


long int compress_img_array ()
{
    unsigned char *jpegBuf;
    long int totalSize = 0;
    for (int i = 0; i < NUM_IMG; i += 1)
    {
        trgb2jpeg(imgArray[i], imgHeaderArray[i].width, imgHeaderArray[i].height, 75, &jpegBuf, &(imgHeaderArray[i].size));
        free (jpegBuf);
        totalSize += imgHeaderArray[i].size;
    }
    return totalSize;
}



int main()
{
    char cwd[128];
    getcwd(cwd,sizeof(cwd));
    read_dir_img (cwd);
    clock_t start = clock ();
    for (int j = 0; j < 5; j++)
    {
        long int totalSize = compress_img_array ();
    }
    clock_t end = clock ();
    double duration = (end - start) / 5;
    printf ("Average duration for compressing %d images is %lf miliseconds.\n", NUM_IMG, duration);

}