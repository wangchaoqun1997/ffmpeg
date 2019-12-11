#include <cstdio>
#include <malloc.h>



void simplest_yuv420_split(const char *url,int w,int h,int num) {
	FILE* fp = fopen(url,"rb+");
	FILE* fp1 = fopen("output_420_y.y", "wb+");
	FILE* fp2 = fopen("output_420_u.y", "wb+");
	FILE* fp3 = fopen("output_420_v.y", "wb+");

	unsigned char* pic = (unsigned char*)malloc(w * h * 3 / 2);

	for (int i = 0; i < num; i++) {
		fread(pic,1, w * h * 3 / 2,fp);

		//Y
		fwrite(pic, 1, w * h, fp1);
		//U
		fwrite(pic+ w * h, 1, w * h/4, fp2);
		//V
		fwrite(pic+ w * h+ w * h / 4, 1, w * h / 4, fp3);
	}
}


void simplest_rgb24_split(const char* url, int w, int h, int num) {
	FILE* fp = fopen(url,"rb+");

	FILE* fp1 = fopen("output_r.y", "wb+");
	FILE* fp2 = fopen("output_g.y", "wb+");
	FILE* fp3 = fopen("output_b.y", "wb+");

	unsigned char* pic = (unsigned char*)malloc(w * h * 3 );
	const char pixO = 255;
	const char pix1 = 255;
	const char pix2 = 0;

	for (int i = 0; i < num; i++) {
		fread(pic,1,w*h*3,fp);

		for (int j = 0; j < w * h *3; j+=3) {
			//R
			fwrite(&pixO, 1, 1, fp1);
			fwrite(&pix1, 1, 1, fp1);
			fwrite(&pix2, 1, 1, fp1);

			////G
			//fwrite(pic + j + 1, 1, 1, fp2);
			////B
			//fwrite(pic + j + 2, 1, 1, fp3);
		}
	}

	free(pic);
	fclose(fp);
	fclose(fp1);
	fclose(fp2);
	fclose(fp3);
	
}


void simplest_rgb24_colorbar(int w, int h, const char *url_out) {
	

	FILE* fp1 = fopen(url_out, "wb+");

	unsigned char* buffer = (unsigned char*)malloc(w * h * 3);
	
	const unsigned char pixO = 255;
	const unsigned char pix1 = 255;
	const unsigned char pix2 = 0;

	//fread(pic, 1, w * h * 3, fp);

	//for (int j = 0; j < w * h * 3; j += 3) {
	//	//R
	//	//fwrite(&pixO, 1, 1, fp1);
	//	//fwrite(&pix1, 1, 1, fp1);
	//	//fwrite(&pix2, 1, 1, fp1);

	//	////G
	//	//fwrite(pic + j + 1, 1, 1, fp2);
	//	////B
	//	//fwrite(pic + j + 2, 1, 1, fp3);

	//	buffer[j] = 255;
	//	buffer[j+1] = 255;
	//	buffer[j+2] = 0;

	//}

	for (int j = 0; j < h; j++) {
		for (int i = 0; i < w; i+=1) {
			buffer[(j * w + i) * 3 + 0] = 0;
			buffer[(j * w + i) * 3 + 1] =0;
			buffer[(j * w + i) * 3 + 2] = 255;
		}
	}

	fwrite(buffer,w*h*3, 1,fp1);

	free(buffer);
	fclose(fp1);

}

int simplest_rgb24_colorbar1(int width, int height,const char* url_out) {

	unsigned char* data = NULL;
	int barwidth;
	char filename[100] = { 0 };
	FILE* fp = NULL;
	int i = 0, j = 0;

	data = (unsigned char*)malloc(width * height * 3);
	barwidth = width / 8;

	if ((fp = fopen(url_out, "wb+")) == NULL) {
		printf("Error: Cannot create file!");
		return -1;
	}

	for (j = 0; j < height; j++) {
		for (i = 0; i < width; i++) {
			int barnum = i / barwidth;
			switch (barnum) {
			case 0: {
				data[(j * width + i) * 3 + 0] = 255;
				data[(j * width + i) * 3 + 1] = 255;
				data[(j * width + i) * 3 + 2] = 255;
				break;
			}
			case 1: {
				data[(j * width + i) * 3 + 0] = 255;
				data[(j * width + i) * 3 + 1] = 255;
				data[(j * width + i) * 3 + 2] = 0;
				break;
			}
			case 2: {
				data[(j * width + i) * 3 + 0] = 0;
				data[(j * width + i) * 3 + 1] = 255;
				data[(j * width + i) * 3 + 2] = 255;
				break;
			}
			case 3: {
				data[(j * width + i) * 3 + 0] = 0;
				data[(j * width + i) * 3 + 1] = 255;
				data[(j * width + i) * 3 + 2] = 0;
				break;
			}
			case 4: {
				data[(j * width + i) * 3 + 0] = 255;
				data[(j * width + i) * 3 + 1] = 0;
				data[(j * width + i) * 3 + 2] = 255;
				break;
			}
			case 5: {
				data[(j * width + i) * 3 + 0] = 255;
				data[(j * width + i) * 3 + 1] = 0;
				data[(j * width + i) * 3 + 2] = 0;
				break;
			}
			case 6: {
				data[(j * width + i) * 3 + 0] = 0;
				data[(j * width + i) * 3 + 1] = 0;
				data[(j * width + i) * 3 + 2] = 255;

				break;
			}
			case 7: {
				data[(j * width + i) * 3 + 0] = 0;
				data[(j * width + i) * 3 + 1] = 0;
				data[(j * width + i) * 3 + 2] = 0;
				break;
			}
			}

		}
	}
	fwrite(data, width * height * 3, 1, fp);
	fclose(fp);
	free(data);

	return 0;
}


void main() {
	//simplest_yuv420_split("video_dstXXXXXXXXXXXX.yuv",384,288,1);
	//simplest_rgb24_split("baidu.rgb",339,202,1);
	simplest_rgb24_colorbar(640,360,"rgb24_colorbar.rgb");
	printf("==================");
}