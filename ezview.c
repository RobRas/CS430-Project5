#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct Pixel {
	unsigned char r, g, b;
} Pixel;

char convertFrom;
char convertTo;

int width, height;

int maxColorValue;

Pixel* pixmap;
FILE* fh;

void getConversionType(char* arg) {
	if (!strcmp(arg, "3") && !strcmp(arg, "6")) {
		fprintf(stderr, "Error: Target needs to be 3 or 6.\n");
		exit(1);
	}
	
	convertTo = arg[0];
}

void getPPMFileType() {
	char PPMFileType [4];
	
	if (fgets(PPMFileType, 4, fh) != NULL) {
		if (!strcmp(PPMFileType, "P3\n")) {
			convertFrom = '3';
		} else if (!strcmp(PPMFileType, "P6\n")) {
			convertFrom = '6';
		} else {
			fprintf(stderr, "Error: Not a PPM file. Incompatible file type.\n");
			exit(1);
		}
	}
}

void getWidthAndHeight() {
	char value[20];
	
	while ((value[0] = fgetc(fh)) == '#') {	// Line is a comment
		while (fgetc(fh) != '\n');	// Ignore the line
	}
	
	if (!isdigit(value[0])) {
		fprintf(stderr, "Error: Image width is not valid.\n");
		exit(1);
	}
	
	int i = 1;
	while ((value[i] = fgetc(fh)) != ' ') {
		if (!isdigit(value[i])) { 
			fprintf(stderr, "Error: Image width is not valid.\n");
			exit(1);
		}
		i++;
	}
	value[i] = '\0';
	width = atoi(value);
	
	i = 0;
	while ((value[i] = fgetc(fh)) != '\n') {
		if (!isdigit(value[i])) {
			fprintf(stderr, "Error: Image height is not valid.\n");
			exit(1);
		}
		i++;
	}
	value[i] = '\0';
	height = atoi(value);
}

void getMaxColorValue() {
	char value[4];
	
	for (int i = 0; i < 4; i++) {
		value[i] = fgetc(fh);
		if (value[i] == '\n') {
			value[i] = '\0';
			break;
		} else if (!isdigit(value[i])) {
			fprintf(stderr, "Error: Value must be a digit.\n");
			exit(1);
		}
	}
	
	maxColorValue = atoi(value);
	
	if (maxColorValue > 255) {
		fprintf(stderr, "Error: Not a PPM file. Max color value too high.\n");
		exit(1);
	} else if (maxColorValue < 1) {
		fprintf(stderr, "Error: Not a PPM file. Max color value too low.\n");
		exit(1);
	}
}
void getP3Value(unsigned char* outValue) {
	unsigned char value[4];
	int rgbValue;
	
	for (int i = 0; i < 4; i++) {
		value[i] = fgetc(fh);
		if (value[i] == '\n') {
			value[i] = '\0';
			break;
		} else if (!isdigit(value[i])) {
			fprintf(stderr, "Error: Value must be a digit.\n");
			exit(1);
		}
	}
	
	rgbValue = atoi(value);
	if (rgbValue > maxColorValue) {
		fprintf(stderr, "Error: Color value exceeding max.\n");
		exit(1);
	}
	
	*outValue = rgbValue;
}
void parseP3() {
	for (int i = 0; i < width * height; i++) {	
		getP3Value(&pixmap[i].r);
		getP3Value(&pixmap[i].g);
		getP3Value(&pixmap[i].b);
	}
}

void parseP6() {
	fread(pixmap, sizeof(Pixel), width * height, fh);
}

void loadPPM() {
	getWidthAndHeight();
	pixmap = malloc(sizeof(Pixel) * width * height);
	getMaxColorValue();
	if (convertFrom == '3') {
		parseP3();
	} else if (convertFrom == '6') {
		parseP6();
	}
}

void writeP3() {
	fprintf(fh, "P3\n# Converted with Robert Rasmussen's ppmrw\n%d %d\n%d\n", width, height, maxColorValue);
	for (int i = 0; i < width * height; i++) {
		fprintf(fh, "%d\n%d\n%d\n", pixmap[i].r, pixmap[i].g, pixmap[i].b);
	}
}

void writeP6() {
	fprintf(fh, "P6\n# Converted with Robert Rasmussen's ppmrw\n%d %d\n%d\n", width, height, maxColorValue);
	fwrite(pixmap, sizeof(Pixel), width*height, fh);
}

int main(int argc, char* argv[]) {
	if (argc != 4) {
		fprintf(stderr, "Usage: ppmrw target input output.\n");
		return 1;
	}
	
	getConversionType(argv[1]);
	
	fh = fopen(argv[2], "rb");
	if (fh == NULL) {
		fprintf(stderr, "Error: Input file not found.\n");
		return 1;
	}
	
	getPPMFileType();
	
	loadPPM();
	
	fclose(fh);
	
	if (convertTo == '3') {
		fh = fopen(argv[3], "w");
	} else if (convertTo == '6') {
		fh = fopen(argv[3], "wb");
	}
	if (fh == NULL) {
		fprintf(stderr, "Error: Output file not found.\n");
		return 1;
	}
	
	if (convertTo == '3') {
		writeP3();
	} else if (convertTo = '6') {
		writeP6();
	}
	
	fclose(fh);
	free(pixmap);
	
	return 0;
}