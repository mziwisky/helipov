// heliBMP.cpp
//
// Convert a bitmap to a bytecode string that can be copied and
// pasted into the AVR code (helilights.c).
//
// Part of the HeliPOV project.
// http://mziwisky.wordpress.com


#include <iostream>
#include <math.h>
#include <stdint.h>
#include <cstdio>
#include "EasyBMP.h"
using namespace std;

//-------TODO:-------
// -- Allow numLEDs and numSlices to be something besides 32 and 256
// -- 
//-------------------

// ------------
//  STRUCTURES
// ------------
struct RadialPixel {
	int r;
	int th;
	bool lit;
};

// ------------
//  PROTOTYPES
// ------------
bool isBlack (int x, int y);
void setRadialPixel (RadialPixel* rp);
void makeByteCode (RadialPixel rp[], int ledOrder[], FILE* output);
//void calcGeometry (double ringID, double ringOD, 
//					int BMPheight, int BMPwidth, 
//					int numLEDs, int numSlices);
void calcGeometry();

// ---------
//  GLOBALS
// ---------
BMP input;
int height;					// height (pixels) of input .bmp
int width;					// width (pixels) of input .bmp
int depth;					// bit depth of input .bmp
double ringID;				// LED ring inner dia in mm
double ringOD;				// LED ring outer dia in mm
int numLEDs;				// Number of LEDs on a blade
int numSlices;				// Number of "slices," i.e. times the LEDs change in one rotation
int rotation;				// number of slices to rotate image CW
bool mirror;				// Bottom blade image must be mirrored
double radialSpacing;		// how many pixels between r=n and r=n+1
double thetaSpacing;		// how many radians between th=k and th=k+1
double IRpix;				// inner radius in pixels

int main( int argc, char* argv[] )
{
    if( argc != 4 )
	{
		cout << "Usage: " << argv[0] << " (-t|-b) <inputfile.bmp> <outputfile.txt>"
		   	<< endl << endl;
		return 1;
	}
	
	//eventually, take these as parameters
	ringID = 128;
	ringOD = 686;
	numLEDs = 32;
	numSlices = 256;
	int *ledOrder = NULL;

	if( string(argv[1]) == "-t" ) {
		mirror = false;
		rotation = 0;
		int arr[] = {16,18,20,22,24,26,28,30,17,19,21,23,25,27,29,31,
							15,13,11,9,7,5,3,1,14,12,10,8,6,4,2,0};
		ledOrder = &arr[0];
	}
	else if( string(argv[1]) == "-b" ) {
		mirror = true;
		rotation = 0;
		int arr[] = {30,28,26,24,22,20,18,16,31,29,27,25,23,21,19,17,
							1,3,5,7,9,11,13,15,0,2,4,6,8,10,12,14};
		ledOrder = &arr[0];
	}
	else {
		cout << "Usage: " << argv[0] << " (-t|-b) <inputfile.bmp> <outputfile.txt>"
		   	<< endl << endl;
		return 1;
	}

	// ledOrder[0] = 16 means to light LED at r=0 (first LED), the 17th bit must be set
	// (set the bit with |= (1 << 16).
	// So ledOrder[31] = 0 means to light LED at r=31 (32nd LED), the 1st bit must be set.
	
	// read the bitmap and make sure it's square and 1 bpp
	if (!input.ReadFromFile(argv[2]))
		return 1;
	
	width = input.TellWidth();
	height = input.TellHeight();
	depth = input.TellBitDepth();

	cout << "BMP info:  " << width << " x " << height << " @ " << depth << " bpp" << endl;
	
	if ( width != height || depth != 1 )
	{
		cout << "Bad file -- must be square and 1 bpp depth." << endl << endl;
		return 1;
	}

	char* outputFile = argv[3];

	//calcGeometry(ringID, ringOD, height, width, numLEDs, numSlices);
	calcGeometry();

	// make an array of radialpixels
	RadialPixel radpix[numLEDs * numSlices];
	
	for (int j=0; j<numSlices; j++) {
		for (int i=0; i<numLEDs; i++) {
			radpix[numLEDs * j + i].th = j;
			radpix[numLEDs * j + i].r = i;
			setRadialPixel(&radpix[numLEDs * j + i]);
		}
	}

	// translate the radialpixels to bytecode
	FILE *out;
	out = fopen (outputFile,"w");
	makeByteCode(radpix, ledOrder, out);
	fclose(out);

	cout << "Byte code written to " << outputFile << endl << endl;
	return 0;
}

bool isBlack (int x, int y)
{
	if (!(input(x,y)->Red))
		return true;
	return false;
}

//void calcGeometry (double ringID, double ringOD, 
//					int BMPheight, int BMPwidth, 
//					int numLEDs, int numSlices)
void calcGeometry()
{
	// NOTE: this assumes BMPheight = BMPwidth, so it only uses BMPwidth. 
	// Eventually, may want to use the smaller of the two to find a square
	// that fits inside the BMP.  But really, it's probably better to require
	// a square BMP.
	double radSpacing_mm = ((ringOD - ringID) / 2) / (numLEDs - 1);
	double pix_per_mm = width / (ringOD + radSpacing_mm);
	radialSpacing = (width - ringID * pix_per_mm - 1) / (2 * numLEDs - 1);
	IRpix = ringID * pix_per_mm / 2;
	thetaSpacing = 2 * 3.14159265 / numSlices;
}

void setRadialPixel (RadialPixel* rp)
{
	// find polar coordinate of the pixel in the center of the rp
	double actualR = IRpix + rp->r * radialSpacing;
	double actualTh;
	if (!mirror)
		actualTh = -(rp->th + numSlices/4 + rotation) * thetaSpacing;
	else
		actualTh = (rp->th - numSlices/4 - rotation) * thetaSpacing;

	// translate to bmp coordinates
	int x = (int)(actualR * cos(actualTh));
	int y = (int)(actualR * sin(actualTh));
	
	// Shift reference point from the center of the bmp to the top left corner
	x += width/2;
	y = (height/2-y);

	if (isBlack(x,y))
		rp->lit = true;
	else
		rp->lit = false;
}

void makeByteCode (RadialPixel rp[], int ledOrder[], FILE* output)
{
	// NOTE that this function assumes numLEDs = 32, so even though that number
	// is used like a variable throughout this program, it actually HAS to be 32
	// in order for this to work. Someday I might want to accept any multiple of 8,
	// or even any number at all.  SAME FOR numSlices -- assumes it's 256!

	// first 4 bytes, first slice.
	// since RadialPixels in rp are already in order of (r,th) =  (0,0), (1,0),
	// (2,0) ... (31,0), (0,1), (1,1), (2,1), ... (30,255), (31,255) -- I can just
	// address them sequentially.
	for (int j=0; j<256; j++) { //loop over the whole image
		uint32_t tmp = 0x00000000;
		for (int i=0; i<32; i++) { //loop over a slice
			if(rp[32*j + i].lit)
				tmp |= (1 << ledOrder[i]);
		}
		//Write the 32-bit value as four bytes.
	    for (int i=24; i>-1; i-=8)
        	fprintf(output, "0x%02X,", (uint8_t)(tmp >> i));
		if (j%4==3)
			fprintf(output, "\n");
	}
}

