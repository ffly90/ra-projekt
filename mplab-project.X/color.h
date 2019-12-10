// FROM: https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
typedef struct 
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char w; // added by Johannes Staib
} RgbColor;

typedef struct 
{
    unsigned char h;
    unsigned char s;
    unsigned char v;
} HsvColor;

RgbColor HsvToRgb(HsvColor hsv);
HsvColor RgbToHsv(RgbColor rgb);
