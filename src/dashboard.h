#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "CompositeGraphics.h"
#include "graphics_utils.h"
#include "vehicle_data.h"

// Dashboard rendering functions
void initDashboard(CompositeGraphics* gfx);

// Individual component drawing functions
void drawOil(int cx, int cy, bool oilValue);
void drawGlowPlug(int cx, int cy, bool on);
void drawSpeed(int x, int y, int value);
void drawCoolant(int cx, int cy, int height, int value);
void drawFuel(int cx, int cy, int height, int value);

// Gauge drawing functions
void drawVerticalBarGauge(int cx, int cy, int height, int value, int minVal, int maxVal, int color);
void drawVerticalGauge(int cx, int cy, int height, int value, int minVal, int maxVal, const char* label, bool isCriticalValue);
void drawSemiCircularGauge(int cx, int cy, int radius, int value, int minVal, int maxVal, int startAngle, int endAngle, const char* label);
void drawSemiCircularGaugeReverse(int cx, int cy, int radius, int value, int minVal, int maxVal, int startAngle, int endAngle, const char* label);

// Helper functions
void drawGaugeCircle(int cx, int cy, int radius, bool oilGauge);
void drawGaugeLabel(int cx, int cy, int height, const char* label);
void drawGaugeValue(int cx, int cy, int height, int value, bool oilGauge);
bool getBlinkState(bool critical);

// Main dashboard drawing function
void drawDashboard();

// Demo mode functions
void randomizeValues();
extern bool IS_DEMO;

#endif
