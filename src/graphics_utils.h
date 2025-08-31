#ifndef GRAPHICS_UTILS_H
#define GRAPHICS_UTILS_H

#include "CompositeGraphics.h"

// Color constants
extern const int COLOR_BLACK;
extern const int COLOR_DARK;
extern const int COLOR_WHITE;

// Basic drawing functions
void drawLine(int x0, int y0, int x1, int y1, int color);
void fillCircle(int cx, int cy, int r, int color);
void drawCircle(int cx, int cy, int r, int color);

// Segment and digit drawing for speedometer
void drawSegment(int x, int y, int scale, bool horizontal, int color);
void drawBigDigit(int x, int y, int digit, int scale, int color);
void drawBigNumber(int x, int y, int number, int scale, int color);

// Icon drawing functions
void drawOilIcon(int cx, int cy, int color);
void drawGlowPlugIcon(int cx, int cy, int color);

// Initialize graphics utilities with the graphics object
void initGraphicsUtils(CompositeGraphics* gfx);

#endif
