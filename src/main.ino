#include "esp_pm.h"
#include "CompositeGraphics.h"
#include "CompositeColorOutput.h"
#include "font6x8.h"
#include <soc/rtc.h>
#include <math.h>
#include <stdlib.h> // For rand()
#include <time.h>   // For time()

CompositeGraphics graphics(CompositeColorOutput::XRES, CompositeColorOutput::YRES, 0);
CompositeColorOutput composite(CompositeColorOutput::NTSC);
Font<CompositeGraphics> font(6, 8, font6x8::pixels);

// --- Dashboard Data ---
int speed = 0;
int coolantTemp = 0;
int fuelLevel = 0;
bool oilLow = false; // LOW = critical, HIGH = ok
bool glowPlugOn = true;

// Set to true to make the display cycle the values.
bool IS_DEMO = false;

unsigned long lastUpdateTime = 0; // New variable to track last update time
const unsigned long UPDATE_INTERVAL_MS = 1000; // New constant for update interval (1 second)

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
    }
}

// Critical thresholds
const int LOW_FUEL = 10;
const int OPTIMUM_TEMP_MIN = 85;
const int OPTIMUM_TEMP_MAX = 95;

// Grayscale colors
const int COLOR_BLACK  = 0;
const int COLOR_DARK   = 20;  // OK / safe
const int COLOR_WHITE  = 63;  // Critical / LOW oil

// ------------------ Helper Functions ------------------
void drawLine(int x0, int y0, int x1, int y1, int color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (true) {
        graphics.dotFast(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void fillCircle(int cx, int cy, int r, int color) {
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x*x + y*y <= r*r) graphics.dotFast(cx + x, cy + y, color);
        }
    }
}

void drawCircle(int cx, int cy, int r, int color) {
    for (int angle=0; angle<360; angle++) {
        float rad = angle * M_PI / 180.0;
        int x = cx + cos(rad)*r;
        int y = cy + sin(rad)*r;
        graphics.dotFast(x, y, color);
    }
}

// ------------------ Gauge Helpers ------------------
bool isCritical(int value, int safeMin, int safeMax, bool twoSided=false, bool oilGauge=false, bool oilValue=false) {
    if(oilGauge) return oilValue; // LOW = critical
    if(twoSided) return value < safeMin || value > safeMax;
    return value < safeMin;
}

// Returns true if the gauge should be visible (handles blinking)
bool getBlinkState(bool critical) {
    if(!critical) return true;
    long t = millis();
    return ((t/300) % 2 == 0);
}

// Draws outer rim and optional center cap
void drawGaugeCircle(int cx, int cy, int radius, bool oilGauge) {
    if(!oilGauge) fillCircle(cx, cy, 4, COLOR_WHITE); // center cap for needles
    drawCircle(cx, cy, radius + 3, COLOR_WHITE);       // outer rim
}

// Draws the label above the gauge
void drawGaugeLabel(int cx, int cy, int height, const char* label)
{
    graphics.setTextColor(COLOR_WHITE);
    graphics.setCursor(cx - (strlen(label) * font.xres)/2, cy - height/2 - 20);
    graphics.print(label);
}

// Draws the value below the gauge
void drawGaugeValue(int cx, int cy, int height, int value, bool oilGauge)
{
    graphics.setTextColor(COLOR_WHITE);
    if(oilGauge) {
        graphics.setCursor(cx - (strlen("LOW")*font.xres)/2, cy + height/2 + 10);
        graphics.print(value ? "LOW" : "HIGH");
    } else {
        graphics.setCursor(cx - (strlen("100")*font.xres)/2, cy + height/2 + 10);
        graphics.print(value);
    }
}

// Helper function for mapping values
long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Draws a vertical bar gauge with horizontal filling bars (terminal style)
void drawVerticalBarGauge(int cx, int cy, int height, int value, int minVal, int maxVal, int color) {
    // Fill with black first to clear previous state
    graphics.fillRect(cx - 7, cy - height/2, 15, height, COLOR_BLACK);

    // Draw the white outline
    graphics.fillRect(cx - 7, cy - height/2 + 7, 1, height - 7, COLOR_WHITE);    // Left line (shortened for rounded top)
    graphics.fillRect(cx + 6, cy - height/2 + 7, 1, height - 7, COLOR_WHITE);    // Right line (shortened for rounded top)
    graphics.fillRect(cx - 10, cy + height/2 - 1, 21, 1, COLOR_WHITE);   // Bottom line (wider base)
    
    // Draw rounded top cap
    int topY = cy - height/2;
    for (int angle = 0; angle <= 180; angle += 5) {
        float rad = angle * M_PI / 180.0;
        int x = cx + cos(rad) * 7;
        int y = topY + 7 - sin(rad) * 7;
        graphics.dotFast(x, y, COLOR_WHITE);
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
        graphics.fillRect(cx - 6, barY, 12, barHeight, color);
    }
}

// Draws a more vibrant and detailed oil can icon
void drawOilIcon(int cx, int cy, int color) {
    // Main body with rounded corners effect
    graphics.fillRect(cx-8, cy-6, 16, 12, color);
    
    // Spout (longer and more prominent)
    graphics.fillRect(cx+8, cy-4, 6, 8, color);
    
    // Handle on top
    graphics.fillRect(cx-6, cy-8, 12, 2, color);
    
    // Oil drop below spout (larger and more visible)
    for(int y=0; y<6; y++){
        for(int x=-y; x<=y; x++){
            graphics.dotFast(cx+10+x, cy+10+y, color);
        }
    }
    
    // Add some highlights for depth
    graphics.fillRect(cx-6, cy-4, 2, 2, COLOR_WHITE);
    graphics.fillRect(cx+10, cy-2, 2, 2, COLOR_WHITE);
}

void drawGlowPlugIcon(int cx, int cy, int color) {
    // Main glow plug body (vertical cylinder)
    graphics.fillRect(cx-3, cy-8, 6, 16, color);
    
    // Top connector/terminal
    graphics.fillRect(cx-4, cy-10, 8, 4, color);
    
    // Heating element (zigzag pattern in the middle)
    graphics.fillRect(cx-2, cy-4, 4, 2, color);
    graphics.fillRect(cx-1, cy-1, 2, 2, color);
    graphics.fillRect(cx-2, cy+2, 4, 2, color);
    
    // Bottom threads
    graphics.fillRect(cx-4, cy+8, 8, 2, color);
    
    // Glow effect (sparkles around the plug)
    graphics.dotFast(cx-6, cy-6, COLOR_WHITE);
    graphics.dotFast(cx+6, cy-6, COLOR_WHITE);
    graphics.dotFast(cx-6, cy+6, COLOR_WHITE);
    graphics.dotFast(cx+6, cy+6, COLOR_WHITE);
    
    // Center glow dot
    graphics.dotFast(cx, cy, COLOR_WHITE);
}

// ------------------ Metric Draw Functions ------------------
void drawOil(int cx, int cy, bool oilValue) {
    bool critical = isCritical(0,0,0,false,true,oilValue);
    bool show = getBlinkState(critical);
    int col = oilValue ? COLOR_WHITE : COLOR_DARK;

    // Icon above
    if(show) {
      drawOilIcon(cx, cy-20, col);
    }
    // Label in middle
    graphics.setTextColor(COLOR_WHITE);
    graphics.setCursor(cx - (strlen("OIL") * font.xres)/2, cy);
    graphics.print("OIL");
    // Value below (closer to label)
    graphics.setCursor(cx - (strlen("LOW")*font.xres)/2, cy + 10);
    graphics.print(oilValue ? "LOW" : "HIGH");
}

void drawVerticalGauge(int cx, int cy, int height, int value, int minVal, int maxVal, const char* label, bool isCriticalValue) {
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
    graphics.setTextColor(COLOR_WHITE);
    graphics.setCursor(x-(4*font.xres)/2, y+40);
    graphics.print("km/h");
}

// ------------------ Speedometer ------------------
void drawSegment(int x, int y, int scale, bool horizontal, int color) {
    if(horizontal) graphics.fillRect(x,y,scale*3+1,scale,color);
    else graphics.fillRect(x,y,scale,scale*3+1,color);
}

void drawBigDigit(int x,int y,int digit,int scale,int color){
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

void drawBigNumber(int x,int y,int number,int scale,int color){
    char buf[10]; sprintf(buf,"%d",number);
    int len=strlen(buf); int digitWidth=scale*8;
    int totalWidth=len*digitWidth; x-=totalWidth/2;
    for(int i=0;i<len;i++) drawBigDigit(x+i*digitWidth,y,buf[i]-'0',scale,color);
}

void drawGlowPlug(int cx, int cy, bool on) {
    if (on) {
        // Icon above
        drawGlowPlugIcon(cx, cy - 20, COLOR_WHITE);
        // Label in middle
        graphics.setTextColor(COLOR_WHITE);
        graphics.setCursor(cx - (strlen("GLOW") * font.xres)/2, cy);
        graphics.print("GLOW");
        // Value below (closer to label)
        graphics.setCursor(cx - (strlen("PLUG") * font.xres)/2, cy + 10);
        graphics.print("PLUG");
    }
}

// Draws a semi-circular gauge around the speedometer
void drawSemiCircularGauge(int cx, int cy, int radius, int value, int minVal, int maxVal, int startAngle, int endAngle, const char* label) {
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
        graphics.dotFast(x, y, COLOR_WHITE);
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
        graphics.setTextColor(COLOR_WHITE);
        float labelRad = (startAngle + (actualEndAngle - startAngle) / 2.0) * M_PI / 180.0;
        int labelX = cx + cos(labelRad) * (radius + 20);
        int labelY = cy + sin(labelRad) * (radius + 20);
        graphics.setCursor(labelX - (strlen(label) * font.xres)/2, labelY - font.yres/2);
        graphics.print(label);
    }
}

// Draws a semi-circular gauge with reverse filling (starts from bottom, fills upward)
void drawSemiCircularGaugeReverse(int cx, int cy, int radius, int value, int minVal, int maxVal, int startAngle, int endAngle, const char* label) {
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
            graphics.dotFast(x, y, COLOR_WHITE);
        }
        for (int angle = 0; angle <= endAngle; angle += 2) {
            float rad = angle * M_PI / 180.0;
            int x = cx + cos(rad) * radius;
            int y = cy + sin(rad) * radius;
            graphics.dotFast(x, y, COLOR_WHITE);
        }
    } else {
        // Normal case (e.g., 135° to 225°)
        for (int angle = startAngle; angle <= endAngle; angle += 2) {
            float rad = angle * M_PI / 180.0;
            int x = cx + cos(rad) * radius;
            int y = cy + sin(rad) * radius;
            graphics.dotFast(x, y, COLOR_WHITE);
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
        graphics.setTextColor(COLOR_WHITE);
        float labelRad;
        if (wrapsAround) {
            labelRad = ((startAngle + endAngle + 360) / 2.0) * M_PI / 180.0;
        } else {
            labelRad = (startAngle + endAngle) / 2.0 * M_PI / 180.0;
        }
        int labelX = cx + cos(labelRad) * (radius + 20);
        int labelY = cy + sin(labelRad) * (radius + 20);
        graphics.setCursor(labelX - (strlen(label) * font.xres)/2, labelY - font.yres/2);
        graphics.print(label);
    }
}

// ------------------ Dashboard ------------------
void drawDashboard() {
    graphics.begin(COLOR_BLACK);
    int xres = graphics.xres;
    int yres = graphics.yres;

    int bottomBarTopY = yres*0.60;
    int thirdWidth = xres/3;
    int gaugeY = bottomBarTopY+40;

    // Draw Oil (left side) - tight against left edge
    drawOil(20, yres/4, oilLow);

    // Draw Glow Plug (right side) - tight against right edge
    drawGlowPlug(xres - 20, yres/4, glowPlugOn);

    // Draw speedometer at exact center of screen
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

    graphics.end();
}

// ------------------ Setup & Loop ------------------
void setup() {
    esp_pm_lock_handle_t lock;
    esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "compositeLock", &lock);
    esp_pm_lock_acquire(lock);

    composite.init();
    graphics.init();
    graphics.setFont(font);
    Serial.begin(9600);
    srand(time(NULL)); // Seed random number generator
}

void loop() {
    if (IS_DEMO) {
        randomizeValues();
    } else {
        if (Serial.available() > 0) {
            String message = Serial.readStringUntil('\n');
            int colonIndex = message.indexOf(':');
            if (colonIndex != -1) {
                String key = message.substring(0, colonIndex);
                String valueStr = message.substring(colonIndex + 1);
                int value = valueStr.toInt();

                if (key == "SPEED") {
                    speed = value;
                } else if (key == "OIL") {
                    oilLow = value;
                } else if (key == "COOLANT") {
                    coolantTemp = value;
                } else if (key == "FUEL") {
                    fuelLevel = value;
                } else if (key == "GLOW") {
                    glowPlugOn = value;
                }
            }
        }
    }

    drawDashboard();
    composite.sendFrameHalfResolution(&graphics.frame);
}
