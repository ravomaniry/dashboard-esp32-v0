#include "dashboard.h"
#include "CompositeGraphics.h"
#include "font6x8.h"
#include <math.h>
#include <string.h>

// Global graphics object pointer
static CompositeGraphics* graphics = nullptr;
static Font<CompositeGraphics>* font_ptr = nullptr;

// Demo mode
bool IS_DEMO = false;
static unsigned long lastUpdateTime = 0;
const unsigned long UPDATE_INTERVAL_MS = 1000;

// Legacy display data (for compatibility with existing dashboard)
static int speed = 0;
static int coolantTemp = 0;
static int fuelLevel = 0;
static bool oilLow = false;
static bool glowPlugOn = false;

void initDashboard(CompositeGraphics* gfx) {
    graphics = gfx;
}

// Helper function for mapping values
long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void randomizeValues() {
    if( millis() > lastUpdateTime + UPDATE_INTERVAL_MS  ){
        lastUpdateTime = millis();
        
        // Sweep through ranges for easier testing
        static int sweepCounter = 0;
        sweepCounter++;
        
        // Speed: sweep 0-200
        speed = (sweepCounter * 2) % 201;
        
        // Coolant: sweep 60-120
        coolantTemp = 60 + ((sweepCounter * 10) % 61);
        
        // Fuel: sweep 0-100
        fuelLevel = (sweepCounter * 8) % 101;
        
        // Oil: alternate between LOW and HIGH
        oilLow = (sweepCounter / 10) % 2;
        
        // Glow plug: alternate between ON and OFF
        glowPlugOn = (sweepCounter / 15) % 2;
        
        // Update vehicle data structure too
        currentVehicleData.coolantTemp = coolantTemp;
        currentVehicleData.fuelLevel = fuelLevel;
        currentVehicleData.oilWarning = oilLow;
    }
}

bool getBlinkState(bool critical) {
    if(!critical) return true;
    long t = millis();
    return ((t/300) % 2 == 0);
}

void drawGaugeCircle(int cx, int cy, int radius, bool oilGauge) {
    if (!graphics) return;
    
    if(!oilGauge) fillCircle(cx, cy, 4, COLOR_WHITE); // center cap for needles
    drawCircle(cx, cy, radius + 3, COLOR_WHITE);       // outer rim
}

void drawGaugeLabel(int cx, int cy, int height, const char* label) {
    if (!graphics) return;
    
    graphics->setTextColor(COLOR_WHITE);
    graphics->setCursor(cx - (strlen(label) * 6)/2, cy - height/2 - 20);
    graphics->print(label);
}

void drawGaugeValue(int cx, int cy, int height, int value, bool oilGauge) {
    if (!graphics) return;
    
    graphics->setTextColor(COLOR_WHITE);
    if(oilGauge) {
        graphics->setCursor(cx - (strlen("LOW") * 6)/2, cy + height/2 + 10);
        graphics->print(value ? "LOW" : "HIGH");
    } else {
        graphics->setCursor(cx - (strlen("100") * 6)/2, cy + height/2 + 10);
        graphics->print(value);
    }
}

void drawVerticalBarGauge(int cx, int cy, int height, int value, int minVal, int maxVal, int color) {
    if (!graphics) return;
    
    // Fill with black first to clear previous state
    graphics->fillRect(cx - 7, cy - height/2, 15, height, COLOR_BLACK);

    // Draw the white outline
    graphics->fillRect(cx - 7, cy - height/2 + 7, 1, height - 7, COLOR_WHITE);    // Left line (shortened for rounded top)
    graphics->fillRect(cx + 6, cy - height/2 + 7, 1, height - 7, COLOR_WHITE);    // Right line (shortened for rounded top)
    graphics->fillRect(cx - 10, cy + height/2 - 1, 21, 1, COLOR_WHITE);   // Bottom line (wider base)
    
    // Draw rounded top cap
    int topY = cy - height/2;
    for (int angle = 0; angle <= 180; angle += 5) {
        float rad = angle * M_PI / 180.0;
        int x = cx + cos(rad) * 7;
        int y = topY + 7 - sin(rad) * 7;
        graphics->dotFast(x, y, COLOR_WHITE);
    }

    int numBars = 10;
    int barHeight = 3;
    int barSpace = 1;
    int segmentHeight = barHeight + barSpace;

    // Calculate how many bars to fill
    int filledBars = map(value, minVal, maxVal, 0, numBars);
    if (filledBars < 0) filledBars = 0;
    if (filledBars > numBars) filledBars = numBars;

    // Draw filled bars from bottom up
    for (int i = 0; i < filledBars; i++) {
        int barY = cy + height/2 - (i * segmentHeight) - barHeight;
        graphics->fillRect(cx - 6, barY, 12, barHeight, color);
    }
}

void drawOil(int cx, int cy, bool oilValue) {
    if (!graphics) return;
    
    bool critical = isCritical(0,0,0,false,true,oilValue);
    bool show = getBlinkState(critical);
    int col = oilValue ? COLOR_WHITE : COLOR_DARK;

    // Icon above
    if(show) {
      drawOilIcon(cx, cy-20, col);
    }
    // Label in middle
    graphics->setTextColor(COLOR_WHITE);
    graphics->setCursor(cx - (strlen("OIL") * 6)/2, cy);
    graphics->print("OIL");
    // Value below (closer to label)
    graphics->setCursor(cx - (strlen("LOW") * 6)/2, cy + 10);
    graphics->print(oilValue ? "LOW" : "HIGH");
}

void drawVerticalGauge(int cx, int cy, int height, int value, int minVal, int maxVal, const char* label, bool isCriticalValue) {
    if (!graphics) return;
    
    bool critical = isCriticalValue;
    bool show = getBlinkState(critical);
    int col = show ? (critical ? COLOR_WHITE : COLOR_DARK) : COLOR_BLACK;

    // Only draw label if one is provided
    if (strlen(label) > 0) {
        drawGaugeLabel(cx, cy, height, label);
    }
    
    if(show) {
        drawVerticalBarGauge(cx, cy, height, value, minVal, maxVal, col);
    }
    
    // Only draw value if label is provided (to avoid duplicate values)
    if (strlen(label) > 0) {
        drawGaugeValue(cx, cy, height, value, false);
    }
}

void drawCoolant(int cx, int cy, int height, int value) {
    drawVerticalGauge(cx, cy, height, value, 60, 120, "COOL", isCritical(value, OPTIMUM_TEMP_MIN, OPTIMUM_TEMP_MAX, true));
}

void drawFuel(int cx, int cy, int height, int value) {
    drawVerticalGauge(cx, cy, height, value, 0, 100, "FUEL", isCritical(value, LOW_FUEL, 100));
}

void drawSpeed(int x, int y, int value) {
    drawBigNumber(x, y, value, 4, COLOR_WHITE);
    if (graphics) {
        graphics->setTextColor(COLOR_WHITE);
        graphics->setCursor(x-(4*6)/2, y+40);
        graphics->print("km/h");
    }
}

void drawGlowPlug(int cx, int cy, bool on) {
    if (!graphics) return;
    
    if (on) {
        // Icon above
        drawGlowPlugIcon(cx, cy - 20, COLOR_WHITE);
        // Label in middle
        graphics->setTextColor(COLOR_WHITE);
        graphics->setCursor(cx - (strlen("GLOW") * 6)/2, cy);
        graphics->print("GLOW");
        // Value below (closer to label)
        graphics->setCursor(cx - (strlen("PLUG") * 6)/2, cy + 10);
        graphics->print("PLUG");
    }
}

void drawSemiCircularGauge(int cx, int cy, int radius, int value, int minVal, int maxVal, int startAngle, int endAngle, const char* label) {
    if (!graphics) return;
    
    // Use white color for speed gauges
    int color = COLOR_WHITE;
    
    // Handle angle wrapping for the fuel gauge
    int actualEndAngle = endAngle;
    if (endAngle < startAngle) {
        actualEndAngle = endAngle + 360; // Wrap around
    }
    
    // Draw the gauge outline (semi-circle)
    for (int angle = startAngle; angle <= actualEndAngle; angle += 2) {
        float rad = (angle % 360) * M_PI / 180.0;
        int x = cx + cos(rad) * radius;
        int y = cy + sin(rad) * radius;
        graphics->dotFast(x, y, COLOR_WHITE);
    }
    
    // Calculate how many bars to fill based on value
    int totalBars = 20; // Number of bars in the semi-circle
    int filledBars = map(value, minVal, maxVal, 0, totalBars);
    if (filledBars < 0) filledBars = 0;
    if (filledBars > totalBars) filledBars = totalBars;
    
    // Draw filled bars along the semi-circle (only once, no double rendering)
    for (int i = 0; i < filledBars; i++) {
        float progress = (float)i / (totalBars - 1);
        int angle = startAngle + (actualEndAngle - startAngle) * progress;
        float rad = (angle % 360) * M_PI / 180.0;
        
        // Draw a thicker bar perpendicular to the circle
        int x1 = cx + cos(rad) * (radius - 10);
        int y1 = cy + sin(rad) * (radius - 10);
        int x2 = cx + cos(rad) * (radius + 4);
        int y2 = cy + sin(rad) * (radius + 4);
        
        // Draw multiple lines to make the tick thicker
        drawLine(x1, y1, x2, y2, color);
        drawLine(x1+1, y1, x2+1, y2, color);
        drawLine(x1, y1+1, x2, y2+1, color);
    }
    
    // Draw the label positioned around the circle, not above it
    if (strlen(label) > 0) {
        graphics->setTextColor(COLOR_WHITE);
        float labelRad = (startAngle + (actualEndAngle - startAngle) / 2.0) * M_PI / 180.0;
        int labelX = cx + cos(labelRad) * (radius + 20);
        int labelY = cy + sin(labelRad) * (radius + 20);
        graphics->setCursor(labelX - (strlen(label) * 6)/2, labelY - 8/2);
        graphics->print(label);
    }
}

void drawSemiCircularGaugeReverse(int cx, int cy, int radius, int value, int minVal, int maxVal, int startAngle, int endAngle, const char* label) {
    if (!graphics) return;
    
    // Use white color for speed gauges
    int color = COLOR_WHITE;
    
    // Handle angle wrapping for gauges that cross 0°
    bool wrapsAround = (endAngle < startAngle);
    
    // Draw the gauge outline (semi-circle)
    if (wrapsAround) {
        // For angles that wrap around (e.g., 45° to 315°)
        for (int angle = startAngle; angle <= 360; angle += 2) {
            float rad = angle * M_PI / 180.0;
            int x = cx + cos(rad) * radius;
            int y = cy + sin(rad) * radius;
            graphics->dotFast(x, y, COLOR_WHITE);
        }
        for (int angle = 0; angle <= endAngle; angle += 2) {
            float rad = angle * M_PI / 180.0;
            int x = cx + cos(rad) * radius;
            int y = cy + sin(rad) * radius;
            graphics->dotFast(x, y, COLOR_WHITE);
        }
    } else {
        // Normal case (e.g., 135° to 225°)
        for (int angle = startAngle; angle <= endAngle; angle += 2) {
            float rad = angle * M_PI / 180.0;
            int x = cx + cos(rad) * radius;
            int y = cy + sin(rad) * radius;
            graphics->dotFast(x, y, COLOR_WHITE);
        }
    }
    
    // Calculate how many bars to fill based on value
    int totalBars = 20; // Number of bars in the semi-circle
    int filledBars = map(value, minVal, maxVal, 0, totalBars);
    if (filledBars < 0) filledBars = 0;
    if (filledBars > totalBars) filledBars = totalBars;
    
    // Draw filled bars starting from endAngle (bottom) towards startAngle (top)
    for (int i = 0; i < filledBars; i++) {
        float progress = (float)i / (totalBars - 1);
        int angle;
        
        if (wrapsAround) {
            // For wrap-around case, calculate correctly
            int totalSpan = (360 - startAngle) + endAngle;
            int angleFromStart = totalSpan * progress;
            angle = (endAngle - angleFromStart + 360) % 360;
        } else {
            // Normal case - start from endAngle, move toward startAngle
            angle = endAngle - (endAngle - startAngle) * progress;
        }
        
        float rad = angle * M_PI / 180.0;
        
        // Draw a thicker bar perpendicular to the circle
        int x1 = cx + cos(rad) * (radius - 10);
        int y1 = cy + sin(rad) * (radius - 10);
        int x2 = cx + cos(rad) * (radius + 4);
        int y2 = cy + sin(rad) * (radius + 4);
        
        // Draw multiple lines to make the tick thicker
        drawLine(x1, y1, x2, y2, color);
        drawLine(x1+1, y1, x2+1, y2, color);
        drawLine(x1, y1+1, x2, y2+1, color);
    }
    
    // Draw the label positioned around the circle, not above it
    if (strlen(label) > 0) {
        graphics->setTextColor(COLOR_WHITE);
        float labelRad;
        if (wrapsAround) {
            labelRad = ((startAngle + endAngle + 360) / 2.0) * M_PI / 180.0;
        } else {
            labelRad = (startAngle + endAngle) / 2.0 * M_PI / 180.0;
        }
        int labelX = cx + cos(labelRad) * (radius + 20);
        int labelY = cy + sin(labelRad) * (radius + 20);
        graphics->setCursor(labelX - (strlen(label) * 6)/2, labelY - 8/2);
        graphics->print(label);
    }
}

void drawDashboard() {
    if (!graphics) return;
    
    graphics->begin(COLOR_BLACK);
    int xres = graphics->xres;
    int yres = graphics->yres;

    // Update display values from vehicle data or use legacy values for demo mode
    if (!IS_DEMO) {
        coolantTemp = currentVehicleData.coolantTemp;
        fuelLevel = currentVehicleData.fuelLevel;
        oilLow = currentVehicleData.oilWarning;
        // Note: speed is GPS-based in Android app, so we don't use it here
        // For dashboard display, we can show a placeholder or hide it
        speed = 0; // GPS speed not available on ESP32
    }

    int bottomBarTopY = yres*0.60;
    int thirdWidth = xres/3;
    int gaugeY = bottomBarTopY+40;

    // Draw Oil (left side) - tight against left edge
    drawOil(20, yres/4, oilLow);

    // Draw Glow Plug (right side) - tight against right edge  
    // Note: This might need to be replaced with another indicator based on your vehicle
    drawGlowPlug(xres - 20, yres/4, glowPlugOn);

    // Draw speedometer at exact center of screen (placeholder since GPS is on Android)
    int centerX = xres / 2;
    int centerY = yres / 2;
    drawSpeed(centerX, centerY, speed);
    
    // Draw left and right circular speed gauges around the speedometer
    int gaugeRadius = 80;
    
    // Left speed gauge - rotated -45° (tilted counterclockwise, angled inward)
    drawSemiCircularGauge(centerX, centerY, gaugeRadius, speed, 0, 120, 135, 225, "");
    
    // Right speed gauge - rotated +45° (tilted clockwise, angled inward, uses reverse function)
    drawSemiCircularGaugeReverse(centerX, centerY, gaugeRadius, speed, 0, 120, 315, 45, "");
    
    // Draw traditional vertical gauges below (with their own labels)
    drawCoolant(20, gaugeY, 40, coolantTemp); // Tight against left edge
    drawFuel(xres - 20, gaugeY, 40, fuelLevel); // Tight against right edge

    graphics->end();
}
