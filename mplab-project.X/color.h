/*
 * FROM:    STACKOVERFLOW
 * AT:      11. DEC 2019
 * BY USER: Leszek Szary
 *          https://stackoverflow.com/users/2102779/leszek-szary
 * LINK:    https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both/14733008#14733008
*/
typedef struct 
{
    unsigned char g; // order changed by Johannes Staib
    unsigned char r; // order changed by Johannes Staib
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
