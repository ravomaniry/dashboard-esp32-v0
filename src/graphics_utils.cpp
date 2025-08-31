#include "graphics_utils.h"
#include "CompositeGraphics.h"
#include "font6x8.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// Color constants
const int COLOR_BLACK = 0;
const int COLOR_DARK = 20;
const int COLOR_WHITE = 63;

// Global graphics object pointer
static CompositeGraphics* graphics = nullptr;

void initGraphicsUtils(CompositeGraphics* gfx) {
    graphics = gfx;
}

void drawLine(int x0, int y0, int x1, int y1, int color) {
    if (!graphics) return;
    
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (true) {
        graphics->dotFast(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void fillCircle(int cx, int cy, int r, int color) {
    if (!graphics) return;
    
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x*x + y*y <= r*r) graphics->dotFast(cx + x, cy + y, color);
        }
    }
}

void drawCircle(int cx, int cy, int r, int color) {
    if (!graphics) return;
    
    for (int angle=0; angle<360; angle++) {
        float rad = angle * M_PI / 180.0;
        int x = cx + cos(rad)*r;
        int y = cy + sin(rad)*r;
        graphics->dotFast(x, y, color);
    }
}

void drawSegment(int x, int y, int scale, bool horizontal, int color) {
    if (!graphics) return;
    
    if(horizontal) graphics->fillRect(x,y,scale*3+1,scale,color);
    else graphics->fillRect(x,y,scale,scale*3+1,color);
}

void drawBigDigit(int x,int y,int digit,int scale,int color) {
    if (!graphics) return;
    
    bool segments[10][7] = {
        {1,1,1,1,1,1,0},{0,1,1,0,0,0,0},{1,1,0,1,1,0,1},{1,1,1,1,0,0,1},
        {0,1,1,0,0,1,1},{1,0,1,1,0,1,1},{1,0,1,1,1,1,1},{1,1,1,0,0,0,0},
        {1,1,1,1,1,1,1},{1,1,1,1,0,1,1}};
    if(digit<0||digit>9)return;
    int seg_h=scale*4; int seg_v=scale;
    if(segments[digit][0]) drawSegment(x+seg_v,y,scale,true,color);
    if(segments[digit][1]) drawSegment(x+seg_h,y+seg_v,scale,false,color);
    if(segments[digit][2]) drawSegment(x+seg_h,y+seg_h+seg_v,scale,false,color);
    if(segments[digit][3]) drawSegment(x+seg_v,y+seg_h*2,scale,true,color);
    if(segments[digit][4]) drawSegment(x,y+seg_h+seg_v,scale,false,color);
    if(segments[digit][5]) drawSegment(x,y+seg_v,scale,false,color);
    if(segments[digit][6]) drawSegment(x+seg_v,y+seg_h,scale,true,color);
}

void drawBigNumber(int x,int y,int number,int scale,int color) {
    if (!graphics) return;
    
    char buf[10]; sprintf(buf,"%d",number);
    int len=strlen(buf); int digitWidth=scale*8;
    int totalWidth=len*digitWidth; x-=totalWidth/2;
    for(int i=0;i<len;i++) drawBigDigit(x+i*digitWidth,y,buf[i]-'0',scale,color);
}

void drawOilIcon(int cx, int cy, int color) {
    if (!graphics) return;
    
    // Main body with rounded corners effect
    graphics->fillRect(cx-8, cy-6, 16, 12, color);
    
    // Spout (longer and more prominent)
    graphics->fillRect(cx+8, cy-4, 6, 8, color);
    
    // Handle on top
    graphics->fillRect(cx-6, cy-8, 12, 2, color);
    
    // Oil drop below spout (larger and more visible)
    for(int y=0; y<6; y++){
        for(int x=-y; x<=y; x++){
            graphics->dotFast(cx+10+x, cy+10+y, color);
        }
    }
    
    // Add some highlights for depth
    graphics->fillRect(cx-6, cy-4, 2, 2, COLOR_WHITE);
    graphics->fillRect(cx+10, cy-2, 2, 2, COLOR_WHITE);
}

void drawGlowPlugIcon(int cx, int cy, int color) {
    if (!graphics) return;
    
    // Main glow plug body (vertical cylinder)
    graphics->fillRect(cx-3, cy-8, 6, 16, color);
    
    // Top connector/terminal
    graphics->fillRect(cx-4, cy-10, 8, 4, color);
    
    // Heating element (zigzag pattern in the middle)
    graphics->fillRect(cx-2, cy-4, 4, 2, color);
    graphics->fillRect(cx-1, cy-1, 2, 2, color);
    graphics->fillRect(cx-2, cy+2, 4, 2, color);
    
    // Bottom threads
    graphics->fillRect(cx-4, cy+8, 8, 2, color);
    
    // Glow effect (sparkles around the plug)
    graphics->dotFast(cx-6, cy-6, COLOR_WHITE);
    graphics->dotFast(cx+6, cy-6, COLOR_WHITE);
    graphics->dotFast(cx-6, cy+6, COLOR_WHITE);
    graphics->dotFast(cx+6, cy+6, COLOR_WHITE);
    
    // Center glow dot
    graphics->dotFast(cx, cy, COLOR_WHITE);
}
