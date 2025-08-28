// code by bitluni (send me a high five if you like the code)
// Modified for car dashboard with arc gauges & warning colors

#include "esp_pm.h"

#include "CompositeGraphics.h"
#include "CompositeColorOutput.h"
#include "font6x8.h"
#include <soc/rtc.h>
#include <math.h>

// Graphics objects
CompositeGraphics graphics(CompositeColorOutput::XRES, CompositeColorOutput::YRES, 0);
CompositeColorOutput composite(CompositeColorOutput::NTSC);
Font<CompositeGraphics> font(6, 8, font6x8::pixels);

// --- Car Dashboard Data ---
int speed = 0;
int coolantTemp = 80;
int fuelLevel = 75;
bool oilWarning = false;

// Critical thresholds
const int CRITICAL_TEMP = 105;
const int LOW_FUEL = 10;
const int OPTIMUM_TEMP_MIN = 85;
const int OPTIMUM_TEMP_MAX = 95;

// --- Colors ---
const int COLOR_BLACK   = 0;
const int COLOR_WHITE   = 63;
const int COLOR_GREY    = 32;
const int COLOR_RED     = 8;
const int COLOR_ORANGE  = 20;
const int COLOR_YELLOW  = 40;
const int COLOR_GREEN   = 50;

// ------------------ Helper Functions ------------------

// Draw segment of big digit
void drawSegment(int x, int y, int scale, bool horizontal, int color) {
    if (horizontal) {
        graphics.fillRect(x, y, scale * 3 + 1, scale, color);
    } else {
        graphics.fillRect(x, y, scale, scale * 3 + 1, color);
    }
}

// Draws a single large 7-seg style digit
void drawBigDigit(int x, int y, int digit, int scale, int color) {
    bool segments[10][7] = {
        {1, 1, 1, 1, 1, 1, 0}, // 0
        {0, 1, 1, 0, 0, 0, 0}, // 1
        {1, 1, 0, 1, 1, 0, 1}, // 2
        {1, 1, 1, 1, 0, 0, 1}, // 3
        {0, 1, 1, 0, 0, 1, 1}, // 4
        {1, 0, 1, 1, 0, 1, 1}, // 5
        {1, 0, 1, 1, 1, 1, 1}, // 6
        {1, 1, 1, 0, 0, 0, 0}, // 7
        {1, 1, 1, 1, 1, 1, 1}, // 8
        {1, 1, 1, 1, 0, 1, 1}  // 9
    };

    if (digit < 0 || digit > 9) return;
    
    int seg_h = scale * 4;
    int seg_v = scale;

    if (segments[digit][0]) drawSegment(x + seg_v, y, scale, true, color);              // A
    if (segments[digit][1]) drawSegment(x + seg_h, y + seg_v, scale, false, color);    // B
    if (segments[digit][2]) drawSegment(x + seg_h, y + seg_h + seg_v, scale, false, color); // C
    if (segments[digit][3]) drawSegment(x + seg_v, y + seg_h * 2, scale, true, color); // D
    if (segments[digit][4]) drawSegment(x, y + seg_h + seg_v, scale, false, color);    // E
    if (segments[digit][5]) drawSegment(x, y + seg_v, scale, false, color);            // F
    if (segments[digit][6]) drawSegment(x + seg_v, y + seg_h, scale, true, color);     // G
}

void drawBigNumber(int x, int y, int number, int scale, int color) {
    char buf[10];
    sprintf(buf, "%d", number);
    int len = strlen(buf);
    int digitWidth = scale * 8;
    int totalWidth = len * digitWidth;
    x -= totalWidth / 2;

    for (int i = 0; i < len; i++) {
        drawBigDigit(x + i * digitWidth, y, buf[i] - '0', scale, color);
    }
}

// ------------------ Graphics Primitives Missing in CompositeGraphics ------------------

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
            if (x * x + y * y <= r * r) {
                graphics.dotFast(cx + x, cy + y, color);
            }
        }
    }
}

// ------------------ Gauges ------------------

int angleToValue(int angle, int minVal, int maxVal) {
    float ratio = (float)(angle + 120) / 240.0;
    return minVal + ratio * (maxVal - minVal);
}

// Draws an arc gauge with a needle and colored scale
void drawArcGauge(int cx, int cy, int radius, int minVal, int maxVal, int value, 
                  int safeMin, int safeMax, const char* label, bool isCritical) {
    for (int angle = -120; angle <= 120; angle++) {
        int x = cx + cos(angle * M_PI / 180) * radius;
        int y = cy + sin(angle * M_PI / 180) * radius;

        int col = COLOR_GREY;
        int v = angleToValue(angle, minVal, maxVal);

        if (isCritical) {
            col = COLOR_RED; // override if warning
        } else if (v < safeMin) {
            col = COLOR_ORANGE;
        } else if (v > safeMax) {
            col = COLOR_RED;
        } else {
            col = COLOR_GREEN;
        }
        graphics.dotFast(x, y, col);
    }

    // Draw needle
    float ratio = (float)(value - minVal) / (maxVal - minVal);
    float angle = -120 + ratio * 240;
    int nx = cx + cos(angle * M_PI / 180) * (radius - 5);
    int ny = cy + sin(angle * M_PI / 180) * (radius - 5);
    drawLine(cx, cy, nx, ny, isCritical ? COLOR_RED : COLOR_WHITE);

    // Center cap
    fillCircle(cx, cy, 4, isCritical ? COLOR_RED : COLOR_WHITE);

    // Label
    graphics.setTextColor(isCritical ? COLOR_RED : COLOR_WHITE);
    graphics.setCursor(cx - (strlen(label) * font.xres) / 2, cy + radius / 2);
    graphics.print(label);

    // Value
    char buf[8];
    sprintf(buf, "%d", value);
    graphics.setCursor(cx - (strlen(buf) * font.xres) / 2, cy + radius / 2 + font.yres + 2);
    graphics.print(buf);
}

void drawOilGauge(int x, int y) {
    graphics.setCursor(x, y);
    graphics.setTextColor(COLOR_GREY);
    graphics.print("OIL");

    int iconY = y + font.yres + 2;
    int iconColor = oilWarning ? COLOR_RED : COLOR_GREEN;

    // Oil can body
    graphics.fillRect(x + 5, iconY + 5, 15, 15, iconColor);
    // Spout
    graphics.fillRect(x + 15, iconY, 5, 8, iconColor);

    graphics.setTextColor(COLOR_WHITE);
    graphics.setCursor(x, iconY + 22);
    graphics.print(oilWarning ? "WARN" : "OK");
}

// ------------------ Dashboard ------------------

void drawDashboard()
{
    graphics.begin(COLOR_BLACK);

    int xres = graphics.xres;
    int yres = graphics.yres;

    int bottomBarTopY = yres * 0.60;

    // --- Speedometer (big number) ---
    int speedometerCenterY = bottomBarTopY / 2;
    drawBigNumber(xres / 2, speedometerCenterY - 30, speed, 4, COLOR_WHITE);
    graphics.setTextColor(COLOR_WHITE);
    graphics.setCursor(xres / 2 - (4 * font.xres) / 2, speedometerCenterY + 30);
    graphics.print("km/h");

    // --- Bottom Section Gauges ---
    int thirdWidth = xres / 3;
    int gaugeY = bottomBarTopY + 40;

    // Oil
    drawOilGauge(10, bottomBarTopY + 10);

    // Coolant arc
    bool coolantCritical = (coolantTemp > CRITICAL_TEMP);
    drawArcGauge(thirdWidth + thirdWidth / 2, gaugeY, 40, 60, 120, coolantTemp,
                 OPTIMUM_TEMP_MIN, OPTIMUM_TEMP_MAX, "COOL", coolantCritical);

    // Fuel arc
    bool fuelCritical = (fuelLevel <= LOW_FUEL);
    drawArcGauge(xres - thirdWidth / 2, gaugeY, 40, 0, 100, fuelLevel,
                 30, 100, "FUEL", fuelCritical);

    graphics.end();
}

// ------------------ Setup & Loop ------------------

void setup()
{
  // highest clockspeed needed
  esp_pm_lock_handle_t powerManagementLock;
  esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "compositeCorePerformanceLock", &powerManagementLock);
  esp_pm_lock_acquire(powerManagementLock);
  
  // initializing DMA buffers and I2S
  composite.init();
  graphics.init();
  graphics.setFont(font);
}

void loop()
{
  // Simulate changing values
  long time = millis();
  speed = 110 + sin(time * 0.001) * 20;
  coolantTemp = 95 + sin(time * 0.0005) * 15;
  fuelLevel = 50 - (time / 2000) % 50;
  oilWarning = (time / 5000) % 4 == 0;

  drawDashboard();
  composite.sendFrameHalfResolution(&graphics.frame);
}
