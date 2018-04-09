/* 
 * File:   StegoBreak.cpp
 * Author: TG, Rinku Dewri (BMP constants, BMP struct, ReadBMP, GetRGB)
 */

#include <cstdlib>
#include <fstream> //file stream
#include <iostream> //cout
#include <algorithm> //std::includes
#include <vector> //std::vector

using namespace std;

typedef unsigned char byte;

#define BMP_HEADER_SIZE 54
#define BMP_WIDTH_OFFSET 18
#define BMP_HEIGHT_OFFSET 22
#define BMP_BPP_OFFSET 28

typedef struct {
    byte bmp_header[BMP_HEADER_SIZE];   // BMP header
    byte **pixel;                       // pixel data (two dimensional array)
    int width;                          // width of image
    int aligned_width;                  // 4 byte aligned width 
    int height;                         // height of image
    int Bpp;                            // bytes per pixel in image
} BMP; // a BMP image

/////////////////
// Read a BMP image
//  return BMP pointer on success, else NULL
BMP *ReadBMP(const char* filename)
{
    FILE* f = fopen(filename, "rb");

    if(f == NULL) return NULL;

    BMP *img = new BMP;
    
    fread(img->bmp_header, sizeof(byte), BMP_HEADER_SIZE, f); // read the 54-byte header

    // extract image width, height, and bytes per pixels from header
    int width = *(int*)&img->bmp_header[BMP_WIDTH_OFFSET];
    int height = *(int*)&img->bmp_header[BMP_HEIGHT_OFFSET];
    int bytes_per_pixel = (*(short*)&img->bmp_header[BMP_BPP_OFFSET])/8;

    // compute new width so that it is a multiple of 4
    int aligned_width = (width*bytes_per_pixel + 3) & (~3);
    
    // store in structure
    img->width = width;
    img->aligned_width = aligned_width;
    img->height = height;
    img->Bpp = bytes_per_pixel;
    
    // allocate memory for pixel data and read it from file
    img->pixel = new byte *[height];
    for(int i = height - 1; i >= 0; i--) {
        img->pixel[i] = new byte[aligned_width];
        fread(img->pixel[i], sizeof(byte), aligned_width, f); // in A, B, G, R order
    }

    fclose(f);
    return img;
}

/////////////////
// Get color values as RGB from pixel (x,y) in <img>
unsigned int GetRGB(BMP *img, int x, int y) {
    unsigned int rgb = 0;
    // only support 8-bit, 16-bit, and 24-bit BMPs
    if (img->Bpp < 1 || img->Bpp > 3){
        cout << "Unsupported bit-depth in BMP.\n";
        exit(0);
    }
    
    int row = y;            // row in pixel data
    int col = img->Bpp * x; // column in pixel data (each pixel occupies Bpp bytes)
        
    // get red -- three channels (rgb)
    rgb = ((rgb << 8) | img->pixel[row][col+2]);
    // get green -- two channels (bg)
    rgb = ((rgb << 8) | img->pixel[row][col+1]);
    // get blue -- one channel (b)
    rgb = ((rgb << 8) | img->pixel[row][col+0]);
    
    return rgb;
}

/* formatPixel      returns a pixel with channels aligned per user request
 * <pixel>          input pixel we wish to reformat 
 *                  (Should always be RGB, given by GetRGB method)
 * <rPos>           the position (0-2) that we want the red channel
 * <gPos>           the position (0-2) that we want the green channel
 * <bPos>           the position (0-2) that we want the blue channel
 *                  (values of -1 mean we do not include this channel)
 * @return          returns pixel after shifting/removing channels */
unsigned int formatPixel(unsigned int pixel, int rPos, int gPos, int bPos){
    int numChannels = 0; // the number of channels we wish to analyze
    int maxChannels = 3; // the maximum number of channels we will analyze
    unsigned int ret = 0;
    int bitsPerByte = 8;
    
    /* Get our RGB channels from <pixel> */
    int redChan = (pixel & 0x00FF0000) >> 16;
    int greenChan = (pixel & 0x0000FF00) >> 8;
    int blueChan = (pixel & 0x000000FF);
    
    /* Now move channels into their requested positions */
    if(rPos != -1){
        /* Fix our positions: given a value with format 0xRRGGBB, 
         * RR is position 0, GG position 1, and BB position 2 */
        rPos = 2 - rPos; // fix the position
        ret |= redChan << (rPos * bitsPerByte);
        numChannels++;
    }
    if(gPos != -1){
        gPos = 2 - gPos;
        ret |= greenChan << (gPos * bitsPerByte);
        numChannels++;
    }
    if(bPos != -1){
        bPos = 2 - bPos;
        ret |= blueChan << (bPos * bitsPerByte);
        numChannels++;
    }
    ret = ret >> ((maxChannels - numChannels) * 8);// remove any trailing zeros
    return ret;
}

/* oneChannel       read BMP, collect LSBs from channels
 *                  we pass in -1 to one channel to exclude it
 * <bmp>            pointer to BMP we wish to read
 * <redChan>        the first channel we want to read (-1 if excluded)
 * <blueChan>       the second channel we want to read (-1 if excluded)
 * <greenChan>      the third channel we want to read (-1 if excluded)
 * @return          returns a vector of LSBs extracted from input file */
vector<byte> oneChannel(BMP* bmp, int redChan, int blueChan, int greenChan){
    unsigned int tempPixel;
    int numPixelsViewed = 0; 
   
    uint8_t chan1, chan1_lsb;
    vector<byte> buffer;
    byte tempByte;
    byte pixelLSBs[8]; // store LSBs extracted from each pixel as a byte
    
    for(int col = 0; col < bmp->height; col++){
        int numBitsExtracted = 0;
        for(int row = 0; row < bmp->width; row++){
            /* Get pixel and extract only the channels we want */
            tempPixel = formatPixel(GetRGB(bmp, row, col), redChan, blueChan, greenChan);
            /* Isolate each channel */
            chan1 = tempPixel & 0xFF;
            /* Extract LSBs from channels */
            chan1_lsb = chan1 & 0x01;
            /* Store those LSBs as a byte */
            tempByte = chan1_lsb;
            /* Store that byte containing LSBs into byte array */
            pixelLSBs[numBitsExtracted] = tempByte;
            numPixelsViewed++;
            
            /* Store a byte created by all LSBs in the past 8 pixels
             * We find 1 bit per pixel, so we require 8 pixels to get a byte */
            if(numPixelsViewed % 8 == 0 && numPixelsViewed != 0){
                byte combinedLSBs = (pixelLSBs[0] << 7) | (pixelLSBs[1] << 6)
                        | (pixelLSBs[2] << 5) | (pixelLSBs[3] << 4)
                        | (pixelLSBs[4] << 3) | (pixelLSBs[5] << 2)
                        | (pixelLSBs[6] << 1) | pixelLSBs[7];
                buffer.push_back(combinedLSBs);
            }
            
            numBitsExtracted++;
            if(numBitsExtracted == 8){
                numBitsExtracted = 0;
            }
        }
    }
    return buffer;
}

/* twoChannel       read BMP, collect LSBs from channels
 *                  we pass in -1 to one channel to exclude it
 * <bmp>            pointer to BMP we wish to read
 * <redChan>        the first channel we want to read (-1 if excluded)
 * <blueChan>       the second channel we want to read (-1 if excluded)
 * <greenChan>      the third channel we want to read (-1 if excluded)
 * @return          returns a vector of LSBs extracted from input file */
vector<byte> twoChannel(BMP* bmp, int redChan, int blueChan, int greenChan){
    unsigned int tempPixel;
    int numPixelsViewed = 0;
    
    uint8_t chan1, chan2, chan1_lsb, chan2_lsb;
    vector<byte> buffer;
    byte tempByte;
    byte pixelLSBs[4]; // store LSBs extracted from each pixel as a byte
    
    for(int col = 0; col < bmp->height; col++){
        int numBitsExtracted = 0;
        for(int row = 0; row < bmp->width; row++){
            /* Get pixel and extract only the channels we want */
            tempPixel = formatPixel(GetRGB(bmp, row, col), redChan, blueChan, greenChan);
            /* Isolate each channel */
            chan1 = (tempPixel & 0xFF00) >> 8;
            chan2 = tempPixel & 0x00FF;
            /* Extract LSBs from channels */
            chan1_lsb = chan1 & 0x01;
            chan2_lsb = chan2 & 0x01;
            /* Store those LSBs as a byte */
            tempByte = (chan1_lsb << 1) | chan2_lsb;
            /* Store that byte containing LSBs into byte array */
            pixelLSBs[numBitsExtracted] = tempByte;
            numPixelsViewed++;
            
            /* Store a byte created by all LSBs in the past 4 pixels
             * We find 2 bits per pixel, so we require 4 pixels to get a byte */
            if(numPixelsViewed % 4 == 0 && numPixelsViewed != 0){
                byte combinedLSBs = (pixelLSBs[0] << 6) | (pixelLSBs[1] << 4)
                        | (pixelLSBs[2] << 2) | pixelLSBs[3];
                buffer.push_back(combinedLSBs);
            }
            
            numBitsExtracted++;
            if(numBitsExtracted == 4){
                numBitsExtracted = 0;
            }
        }
    }
    return buffer;
}

/* threeChannel     read BMP, collect LSBs from channels
 * <bmp>            pointer to BMP we wish to read
 * <redChan>        the first channel we want to read
 * <blueChan>       the second channel we want to read
 * <greenChan>      the third channel we want to read
 * @return          returns a vector of LSBs extracted from input file */
vector<byte> threeChannel(BMP* bmp, int redChan, int blueChan, int greenChan){
    unsigned int tempPixel;

    uint8_t chan1, chan2, chan3, chan1_lsb, chan2_lsb, chan3_lsb;
    vector<byte> buffer;
    byte tempByte;
    byte pixelLSBs[8]; // store LSBs extracted from each pixel as a byte
    int bitIndex = 0; 
    
    for(int col = 0; col < bmp->height; col++){
        int numBitsExtracted = 0;
        for(int row = 0; row < bmp->width; row++){
            /* Get pixel and extract only the channels we want */
            tempPixel = formatPixel(GetRGB(bmp, row, col), redChan, blueChan, greenChan);
            /* Isolate each channel */
            chan1 = (tempPixel & 0xFF0000) >> 16;
            chan2 = (tempPixel & 0x00FF00) >> 8;
            chan3 = tempPixel & 0x0000FF;
            /* Extract LSBs from channels */
            chan1_lsb = chan1 & 0x01;
            chan2_lsb = chan2 & 0x01;
            chan3_lsb = chan3 & 0x01;
            /* Store those LSBs as a byte */
            tempByte = (chan1_lsb << 2) | (chan2_lsb << 1) | chan3_lsb;
            /* Store that byte containing LSBs into byte array */
            pixelLSBs[bitIndex] = tempByte;
            bitIndex++;
            numBitsExtracted += 3; // extract 3 bits per pixel
            
            /* We read 8 pixels, each containing 3 bits; we must read 8
             * pixels to ensure we can extract an even number of bytes */
            if(numBitsExtracted == 24){
                byte combinedLSB1 = (pixelLSBs[0] << 5) | (pixelLSBs[1] << 2)
                        | (pixelLSBs[2] >> 1);
                byte combinedLSB2 = ( (pixelLSBs[2] & 0b001) << 7) 
                        | (pixelLSBs[3] << 4) | (pixelLSBs[4] << 1)
                        | (pixelLSBs[5] >> 2);
                byte combinedLSB3 = ( (pixelLSBs[5] & 0b011) << 6) | 
                        (pixelLSBs[6] << 3) | pixelLSBs[7];
                
                buffer.push_back(combinedLSB1);
                buffer.push_back(combinedLSB2);
                buffer.push_back(combinedLSB3);
                
                numBitsExtracted = 0;
                bitIndex = 0;
            }
        }
    }
    return buffer;
}

/* constructFile    attempt to build a file from extracted LSBs
 * <contents>       vector of bytes containing LSBs extracted from input file
 * <filename>       the name of file without extension
 * <channels>       the channels which we extracted LSBs
 * @return          returns true if we recognized the file, false otherwise */
bool constructFile(vector<byte> contents, string filename, string channels){
    vector<byte> jpg_header = {0xFF, 0xD8, 0xFF};
    vector<byte> bmp_header = {0x42, 0x4D};
    vector<byte> docx_header = {0x50, 0x4B, 0x03, 0x04, 0x14, 0x00, 0x06, 0x00};
    vector<byte> pdf_header = {0x25, 0x50, 0x44, 0x46};
    vector<byte> mp3_header = {0x49, 0x44, 0x33};  
    string file_type;
    bool recognizedFileType = false;
    
    ofstream fout; // create a file and put our extracted LSBs into it
    fout.open(filename.c_str(), ios::binary | ios::out);
    for(vector<byte>::iterator i = contents.begin(); i != contents.end(); ++i){
        fout << *i;
    }
    fout.close();
    
    /* Look for recognized file headers in our extracted file */
    if(std::includes(contents.begin(), contents.begin()+jpg_header.size(),
            jpg_header.begin(), jpg_header.end())){
        recognizedFileType = true;
        file_type = ".jpg";
    } else if(std::includes(contents.begin(), contents.begin()+bmp_header.size(),
            bmp_header.begin(), bmp_header.end())){
        recognizedFileType = true;
        file_type = ".bmp";
    }  else if(std::includes(contents.begin(), contents.begin()+docx_header.size(),
            docx_header.begin(), docx_header.end())){
        recognizedFileType = true;
        file_type = ".docx";
    } else if(std::includes(contents.begin(), contents.begin()+pdf_header.size(),
            pdf_header.begin(), pdf_header.end())){
        recognizedFileType = true;
        file_type = ".pdf";
    } else if(std::includes(contents.begin(), contents.begin()+mp3_header.size(),
            mp3_header.begin(), mp3_header.end())){
        recognizedFileType = true;
        file_type = ".mp3";
    }
    
    /* If we found a file header, append channels we analyzed to recover file 
     * and the type of file we found */
    if(recognizedFileType){
        string new_name = filename + "-" + channels + file_type;
        rename(filename.c_str(), new_name.c_str());
    } else { // delete the file if we don't find a matching header
        remove(filename.c_str());
    }
    return recognizedFileType;
}

int main(int argc, char** argv) {
    std::string nameString(argv[1]); // construct string from char* 
    BMP* bmp = ReadBMP(nameString.c_str());
    
    nameString.erase(nameString.size()-4, 4); // remove file extension
    
    constructFile(threeChannel(bmp, 0, 1, 2), nameString, "rgb");//RGB
    constructFile(threeChannel(bmp, 0, 2, 1), nameString, "rbg");//RBG  
    constructFile(threeChannel(bmp, 1, 0, 2), nameString, "grb");//GRB
    constructFile(threeChannel(bmp, 2, 0, 1), nameString, "gbr");//GBR
    constructFile(threeChannel(bmp, 1, 2, 0), nameString, "brg");//BRG
    constructFile(threeChannel(bmp, 2, 1, 0), nameString, "bgr");//BGR
    constructFile(twoChannel(bmp, 0, -1, 1), nameString, "rb");//RB
    constructFile(twoChannel(bmp, 0, 1, -1), nameString, "rg");//RG
    constructFile(twoChannel(bmp, 1, 0, -1), nameString, "gr");//GR
    constructFile(twoChannel(bmp, -1, 0, 1), nameString, "gb");//GB
    constructFile(twoChannel(bmp, 1, -1, 0), nameString, "br");//BR
    constructFile(twoChannel(bmp, -1, 1, 0), nameString, "bg");//BG
    constructFile(oneChannel(bmp, 0, -1, -1), nameString, "r");//R
    constructFile(oneChannel(bmp, -1, 0, -1), nameString, "g");//G
    constructFile(oneChannel(bmp, -1, -1, 0), nameString, "b");//B

    return 0;
}
