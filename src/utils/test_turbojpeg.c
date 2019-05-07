#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>
 
// #include "jpeg-utils.h"
// #include "libjpeg/include/turbojpeg.h"
#include <dirent.h>
#include <unistd.h>

#include <turbojpeg.h>

#define NUM_IMG 900


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

    // if ((dir=opendir("pics")) == NULL)
    // {
    //     perror("Open dir \"pic/\" encountered an error...\n");
    //     exit(1);
    // }

    while ((ptr=readdir(dir)) != NULL, i < NUM_IMG)
    {
        // printf ("i = %d\n", i);
        i += 1;
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    ///current dir OR parrent dir
            continue;
        else if(ptr->d_type == 8)    ///file
        {
            // printf ("It is a regular file. File name: %s\n", ptr->d_name);
            FILE *imgFile = NULL;
            unsigned char *jpegBuf = NULL;
            int size = 0;
            imgFile = fopen(ptr->d_name, "rb");
            if (imgFile == NULL)
            {
                printf ("Unable to open such file.\n");
                exit (1);
            }
            if (fseek(imgFile, 0, SEEK_END) < 0 || ((size = ftell(imgFile)) < 0) ||
            fseek(imgFile, 0, SEEK_SET) < 0)
            {
                printf ("File size error.\n");
                exit (1);
            }
            if (size == 0)
            {
                printf ("Size equals to zero");
                exit (1);
            }
            unsigned long jpegSize = (unsigned long)size;
            // unsigned char *jpegBuf = NULL;
            if ((jpegBuf = (unsigned char *)tjAlloc(jpegSize)) == NULL)
            {
                // THROW_UNIX("allocating JPEG buffer");
                printf ("Allocation for jpeg buffer failed");
                exit (1);
            }
            if (fread(jpegBuf, jpegSize, 1, imgFile) < 1)
            {
                // THROW_UNIX("reading input file");
                printf ("Unable to read this file to the buffer");
                exit (1);
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
        // free (jpegBuf);
        totalSize += imgHeaderArray[i].size;
    }
    // free (jpegBuf);
    return totalSize;
}



int main()
{
    char cwd[128];
    long int totalSize = 0;
    int ch_res = chdir ("pics");
    if (ch_res == -1)
    {
        printf ("Unable change directory to \"pic\".\n");
        exit(1);
    }
    getcwd(cwd, sizeof(cwd));
    printf ("Current directory: %s\n", cwd);
    read_dir_img (cwd);
    // clock_t start = clock ();
    struct timespec start, end;
    clock_gettime (CLOCK_MONOTONIC_RAW, &start);
    printf ("compressing %d images.\n", NUM_IMG);
    for (int j = 0; j < 100; j++)
    {
        totalSize = compress_img_array ();
    }
    // clock_t end = clock ();
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    time_t _duration = end.tv_nsec - start.tv_nsec;
    printf ("Start time: %ld\n", start.tv_nsec);
    printf ("End time: %ld\n", end.tv_nsec);
    printf ("Total duration: %ld nanoseconds.\n", _duration);
    long double duration = _duration / 100 / 1000;
    printf ("Average duration for compressing %ld bytes of image is %Lf microseconds.\n", totalSize, duration);
    printf ("Average compression speed is %Lf MBps.\n", totalSize / duration / 1000);

}