// C++ font converter by Hawkeye
// Convert a PNM of 8-bit greyscale image data (0x00-0xff min->max brightness)
// into a 2 pixel-per-byte data structure, that can be directly included in the project

#include <iostream>
#include <fstream>

const char* resname = "icons";

using namespace std;
int main(int argc, char* argv[])
{
   ifstream ifs(argv[1]);

   string version;
   getline(ifs, version);

   bool greymap = (version == "P5");

   string widthStr;
   getline(ifs, widthStr, ' ');
   int width = stoi(widthStr);

   string heightStr;
   getline(ifs, heightStr);
   int height = stoi(heightStr);

   string maxBrightness;
   getline(ifs, maxBrightness);

   //cout << width << " x " << height << "px" << endl;
   //cout << maxBrightness << endl;

   // 4 bit 2pixels per byte output
   // map color from 0x00 - 0xff to a 0-15 range (>> 4), read in two bytes at once and calculate output byte

   cout << "const u16 " << resname << "_width=" << width << ";" << endl;
   cout << "const u16 " << resname << "_height=" << height << ";" << endl;
   cout << "const u8 " << resname << "_pixdata[]={";

   bool comma = false;

   for (int y=0; y<height; y++)
   {  
      for (int x=0; x < width; x+=2)
      {
         unsigned char first = ifs.get(); // red, 1st pixel
         if (!greymap) ifs.get(); // drop green
         if (!greymap) ifs.get(); // drop blue
         unsigned char second = ifs.get(); // red, 2nd pixel
         if (!greymap) ifs.get(); // drop green
         if (!greymap) ifs.get(); // drop blue
         unsigned out = (first & 0xf0) + (second >> 4);

         if (comma)
            cout << ",";
         comma = true;

         cout << out;
      }
   }
   cout << "};" << endl;
}