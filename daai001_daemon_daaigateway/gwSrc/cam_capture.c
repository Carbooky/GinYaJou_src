#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <linux/types.h>
#include <linux/videodev2.h>

#include <errno.h>


#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#include <setjmp.h>
#include "jpeglib.h"

#include "cam_capture.h"
//#include "v4l2grab.h"

//typedef int BOOL;

//#define  TRUE    1
//#define  FALSE    0
//DEL struct v4l2_format fmtack;

// #define FILE_VIDEO     "/dev/video0"

//#define BMP          "image_bmp.bmp"
// #define YUV         "image_yuv.yuv"
// #define JPEG        "image_jpeg.jpg"

//#define  IMAGEWIDTH    320
//#define  IMAGEHEIGHT   240

#define  IMAGEWIDTH    640
#define  IMAGEHEIGHT   480

#define CLEAR(x) memset (&(x), 0, sizeof (x))

static int fd;

unsigned char frame_buffer[IMAGEWIDTH * IMAGEHEIGHT * 3];

static unsigned int n_buffers = 0;

struct buffer {
	void * start;
	unsigned int length;
}* buffers;

pthread_mutex_t m_capture_mutex = PTHREAD_MUTEX_INITIALIZER;

//DEL int cc_init_v4l2(void) {

// ex. devVideo : "/dev/video0"
int cc_init_v4l2(char* devVideo) {
	// int i;
	// int ret = 0;

	//opendev
	if ((fd = open(devVideo, O_RDWR)) == -1) {
		printf("Error opening V4L interface\n");
		return (false);
	}

	static struct v4l2_capability cap;
	struct v4l2_fmtdesc fmtdesc;
	struct v4l2_format fmt;
//	struct v4l2_streamparm setfps;

	//query cap
	if (-1 == ioctl(fd, VIDIOC_QUERYCAP, &cap))
	{
		printf("Error opening device %s: unable to query device.\n", devVideo);
		return (false);
	} else {
		printf("driver:\t\t%s\n", cap.driver);
		printf("card:\t\t%s\n", cap.card);
		printf("bus_info:\t%s\n", cap.bus_info);
		printf("version:\t%d\n", cap.version);
		printf("capabilities:\t%x\n", cap.capabilities);

		if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
				== V4L2_CAP_VIDEO_CAPTURE) {
			printf("Device %s: supports capture.\n", devVideo);
		}

		if ((cap.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING) {
			printf("Device %s: supports streaming.\n", devVideo);
		}
	}

	//emu all support fmt
	fmtdesc.index = 0;
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	printf("Support format:\n");
	while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) != -1) {
		printf("\t%d.%s\n", fmtdesc.index + 1, fmtdesc.description);
		fmtdesc.index++;
	}

	CLEAR(fmt);

	//set fmt
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.height = IMAGEHEIGHT;
	fmt.fmt.pix.width = IMAGEWIDTH;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
		printf("Unable to set format\n");
		return false;
	}
	if (ioctl(fd, VIDIOC_G_FMT, &fmt) == -1) {
		printf("Unable to get format\n");
		return false;
	}
	{
		printf("fmt.type:\t\t%d\n", fmt.type);
		printf("pix.pixelformat:\t%c%c%c%c\n", fmt.fmt.pix.pixelformat & 0xFF,
				(fmt.fmt.pix.pixelformat >> 8) & 0xFF,
				(fmt.fmt.pix.pixelformat >> 16) & 0xFF,
				(fmt.fmt.pix.pixelformat >> 24) & 0xFF);
		printf("pix.height:\t\t%d\n", fmt.fmt.pix.height);
		printf("pix.width:\t\t%d\n", fmt.fmt.pix.width);
		printf("pix.field:\t\t%d\n", fmt.fmt.pix.field);
	}
	//set fps
//	setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//	setfps.parm.capture.timeperframe.numerator = 10;
//	setfps.parm.capture.timeperframe.denominator = 10;

	printf("init %s \t[OK]\n", devVideo);

	return true;
}

int cc_v4l2_grab(void)
{
	// unsigned int n_buffers;

	struct v4l2_requestbuffers req;
	struct v4l2_buffer buf;
	enum v4l2_buf_type type;

	//request for 4 buffers
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
		printf("request for buffers error\n");
	}

	//mmap for buffers
	buffers = malloc(req.count * sizeof(*buffers));
	if (!buffers) {
		printf("Out of memory\n");
		return (false);
	}

	for (n_buffers = 0; n_buffers < req.count; n_buffers++) {
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;
		//query buffers
		if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
			printf("query buffer error\n");
			return (false);
		}

		buffers[n_buffers].length = buf.length;
		//map
		buffers[n_buffers].start = mmap(NULL, buf.length,
				PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
		if (buffers[n_buffers].start == MAP_FAILED ) {
			printf("buffer map error\n");
			return (false);
		}
	}

	//queue
	for (n_buffers = 0; n_buffers < req.count; n_buffers++) {
		buf.index = n_buffers;
		ioctl(fd, VIDIOC_QBUF, &buf);
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl(fd, VIDIOC_STREAMON, &type);

	ioctl(fd, VIDIOC_DQBUF, &buf);

	printf("grab yuyv OK\n");
	return (true);
}



int cc_yuyv_2_rgb888(void) {
	int i, j;
	unsigned char y1, y2, u, v;
	int r1, g1, b1, r2, g2, b2;

	char * pointer;
	pointer = buffers[0].start;
	for (i = 0; i < IMAGEHEIGHT; i++) {
		for (j = 0; j < IMAGEWIDTH / 2; j++) // get 4 bytes(two pixel)，change to rgb 6 bytes(2 pixel)
		{

			y1 = *(pointer + (i * IMAGEWIDTH / 2 + j) * 4);
			u = *(pointer + (i * IMAGEWIDTH / 2 + j) * 4 + 1);
			y2 = *(pointer + (i * IMAGEWIDTH / 2 + j) * 4 + 2);
			v = *(pointer + (i * IMAGEWIDTH / 2 + j) * 4 + 3);

			r1 = y1 + 1.402 * (u - 128);            // v -> u
			g1 = y1 - 0.344 * (u - 128) - 0.714 * (v - 128);
			b1 = y1 + 1.772 * (v - 128);            // u -> v

			r2 = y2 + 1.402 * (u - 128);            // v -> u
			g2 = y2 - 0.344 * (u - 128) - 0.714 * (v - 128);
			b2 = y2 + 1.772 * (v - 128);            // u -> v

			if (r1 > 255)
				r1 = 255;
			else if (r1 < 0)
				r1 = 0;

			if (b1 > 255)
				b1 = 255;
			else if (b1 < 0)
				b1 = 0;

			if (g1 > 255)
				g1 = 255;
			else if (g1 < 0)
				g1 = 0;

			if (r2 > 255)
				r2 = 255;
			else if (r2 < 0)
				r2 = 0;

			if (b2 > 255)
				b2 = 255;
			else if (b2 < 0)
				b2 = 0;

			if (g2 > 255)
				g2 = 255;
			else if (g2 < 0)
				g2 = 0;

			*(frame_buffer + (i * IMAGEWIDTH / 2 + j) * 6) = (unsigned char) b1;
			*(frame_buffer + (i * IMAGEWIDTH / 2 + j) * 6 + 1) =
					(unsigned char) g1;
			*(frame_buffer + (i * IMAGEWIDTH / 2 + j) * 6 + 2) =
					(unsigned char) r1;
			*(frame_buffer + (i * IMAGEWIDTH / 2 + j) * 6 + 3) =
					(unsigned char) b2;
			*(frame_buffer + (i * IMAGEWIDTH / 2 + j) * 6 + 4) =
					(unsigned char) g2;
			*(frame_buffer + (i * IMAGEWIDTH / 2 + j) * 6 + 5) =
					(unsigned char) r2;
		}
	}
	printf("change to RGB OK \n");
	return 0;
}

bool cc_encode_jpeg(char *lpbuf, int width, int height, char* outputFileName)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1];
	int row_stride;
	char *buf = NULL;
	int x;

	// FILE *fptr_jpg = fopen(JPEG, "wb");
	FILE *fptr_jpg = fopen(outputFileName, "w+b");
	if (fptr_jpg == NULL ) {
		printf("Encoder:open file failed!\n");
		return false;
	}

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, fptr_jpg);

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);

	jpeg_set_quality(&cinfo, 80, true);

	jpeg_start_compress(&cinfo, true);

	row_stride = width * 3;
	buf = malloc(row_stride);
	row_pointer[0] = (JSAMPROW)buf;
	while ((int)cinfo.next_scanline < height) {
		for (x = 0; x < row_stride; x += 3) {

			buf[x] = lpbuf[x];
			buf[x + 1] = lpbuf[x + 1];
			buf[x + 2] = lpbuf[x + 2];

		}
		jpeg_write_scanlines(&cinfo, row_pointer, 1);            //critical
		lpbuf += row_stride;
	}

	jpeg_finish_compress(&cinfo);
	fclose(fptr_jpg);
	jpeg_destroy_compress(&cinfo);
	free(buf);
	return true;

}

int cc_xioctl(int fd, int request, void *arg)
{
	int r;

	do
		r = ioctl(fd, request, arg);
	while (-1 == r && EINTR == errno);

	return r;
}


void cc_stop_capturing()
{
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == cc_xioctl(fd, VIDIOC_STREAMOFF, &type)){
		fprintf(stderr, "%s error %d, %s\n", "VIDIOC_STREAMOFF", errno, strerror(errno));
	}

}

void cc_uninit_device()
{
	unsigned int i;
	for (i = 0; i < n_buffers; ++i)
		if (-1 == munmap(buffers[i].start, buffers[i].length))
		{
			fprintf(stderr, "%s error %d, %s\n", "munmap", errno, strerror(errno));
		}

	if(buffers)free(buffers);
}


int cc_close_v4l2(void)
{

	cc_stop_capturing();

	cc_uninit_device();

	if (fd != -1) {
		close(fd);
		fd = -1;
		return (true);
	}
	return (false);
}

bool checkFolderExist(char *pFolder)
{

    bool bRet = false;

    struct stat stFileInfo;
    int intStat = stat(pFolder, &stFileInfo);
printf("debug bbb.0.0.1 >>> intStat = %d\n", intStat);
    if(intStat == 0)
    {
        if(stFileInfo.st_mode & S_IFREG)
        {
            printf("[cc] *** Input is regular file.\n");
        	// to delete
        } else if(stFileInfo.st_mode & S_IFDIR)
        {
            printf("[cc] *** Input is directory.\n");
        	bRet = true;
        }else{
            int status = mkdir(pFolder, S_IRWXU | S_IRWXG | S_IRWXO);
            if(status == 0)
            	bRet = true;
        }
    }else{
        int status = mkdir(pFolder, S_IRWXU | S_IRWXG | S_IRWXO);
        if(status == 0)
        	bRet = true;
    }

    return bRet;
}
///////////////////////////////////////////////////////////
// public function

// ex. devVideoPath : "/dev/video0"
bool CamCapture_startCapture(char* targetFilePath, char* targetFileName, char* devVideoPath)
{
	if(targetFileName == NULL)
		return false;

	pthread_mutex_lock(&m_capture_mutex);

	bool bresult = true;
	char jpgFileName[256];

	if(targetFilePath != NULL || strlen(targetFilePath) > 0)
	{
		if(checkFolderExist(targetFilePath) == false)
			return false;

		if(targetFilePath[strlen(targetFilePath)-1] != '/' )
			sprintf(jpgFileName,"%s/%s",targetFilePath, targetFileName);
		else
			sprintf(jpgFileName,"%s%s",targetFilePath, targetFileName);
	}else{
		sprintf(jpgFileName,"%s", targetFileName);
	}


	if (cc_init_v4l2(devVideoPath) == false) {
		pthread_mutex_unlock(&m_capture_mutex);
		return (false);
	}

	bresult = cc_v4l2_grab();

	cc_yuyv_2_rgb888();                  //yuyv to RGB24

	bresult = cc_encode_jpeg((char*) frame_buffer, IMAGEWIDTH, IMAGEHEIGHT, jpgFileName); //RGB24 to Jpeg

	// printf("save "JPEG"OK... \n");
	if(bresult){
	 printf("save jpgFileName : %s OK... \n",jpgFileName);
	 printf("save %s OK... \n",targetFileName);
	}else{
		 printf("save jpgFileName : %s error... \n",jpgFileName);
		 printf("save %s error... \n",targetFileName);
	}
	cc_close_v4l2();

	pthread_mutex_unlock(&m_capture_mutex);

	return bresult;
}

#if 0
int main(void)
{

	int i;
	bool bret = false;
	char filename[256];
	for(i = 0 ; i < 10; i ++){
		checkFolderExist("tmp_img/");
		sprintf(filename, "image_(%d).jpg", i);
		bret = CamCapture_startCapture("tmp_img/",filename,"/dev/video0");
		usleep(500000);
		printf("--------------------------\n");
	}
	return bret;
}
#endif


pthread_mutex_t time_mutex = PTHREAD_MUTEX_INITIALIZER;

char utcTimeStr[sizeof (struct tm)] = "";
char* Time_getCurrent()
{
        pthread_mutex_lock(&time_mutex);
        time_t timeCurrent;
        struct tm* cTime;

        time(&timeCurrent);
        cTime = gmtime(&timeCurrent);

        strftime(utcTimeStr, sizeof(struct tm), "%FT%T+08:00", cTime);
        pthread_mutex_unlock(&time_mutex);
        return utcTimeStr;
}

long long Time_getCurrentMicrosecond() {
        long long ms = 0;
        struct timeval tv;
        gettimeofday(&tv, NULL );
        ms = tv.tv_sec * 1000000 + tv.tv_usec;
        return ms;
}


