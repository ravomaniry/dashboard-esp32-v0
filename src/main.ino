#include "esp_pm.h"
#include "CompositeGraphics.h"
#include "CompositeColorOutput.h"
#include "font6x8.h"
#include <soc/rtc.h>
#include <math.h>

CompositeGraphics graphics(CompositeColorOutput::XRES, CompositeColorOutput::YRES, 0);
CompositeColorOutput composite(CompositeColorOutput::NTSC);
Font<CompositeGraphics> font(6, 8, font6x8::pixels);

// --- Dashboard Data ---
int speed = 0;
int coolantTemp = 80;
int fuelLevel = 75;
bool oilLow = false; // LOW = critical, HIGH = ok

// Critical thresholds
const int CRITICAL_TEMP = 105;
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

// Draws the label and value text below the gauge
void drawGaugeLabel(int cx, int cy, int radius, const char* label, bool oilGauge, int value)
{
    graphics.setTextColor(COLOR_WHITE);
    graphics.setCursor(cx - (strlen(label) * font.xres)/2, cy + radius/2 - 4);
    graphics.print(label);
    graphics.setCursor(cx - (oilGauge ? strlen("LOW")*font.xres/2 : strlen("100")*font.xres/2), cy + radius/2 + font.yres - 2);
    if(oilGauge) graphics.print(value ? "LOW" : "HIGH");
    else graphics.print(value);
}

// Draws a half-pie gauge fill (for coolant/fuel)
void drawHalfPieFill(int cx, int cy, int radius, int value, int minVal, int maxVal, int color) {
    float ratio = float(value - minVal) / (maxVal - minVal);
    float angleValue = -180 + ratio * 180;
    for(int angle=-180; angle<=angleValue; angle++) {
        float rad = angle * M_PI / 180.0;
        for(int r=0; r<=radius; r++) {
            int x = cx + cos(rad)*r;
            int y = cy + sin(rad)*r;
            graphics.dotFast(x, y, color);
        }
    }
}

// Draws a minimal oil can icon with drop closer to spout
void drawOilIcon(int cx, int cy, int color) {
    graphics.fillRect(cx-8, cy-8, 16, 10, color); // body
    graphics.fillRect(cx+8, cy-6, 4, 6, color);   // spout
    // drop closer to spout
    for(int y=0;y<4;y++){
        for(int x=-y;x<=y;x++){
            graphics.dotFast(cx+8+x, cy+8+y, color);
        }
    }
}

// ------------------ Metric Draw Functions ------------------
void drawOil(int cx, int cy, int radius, bool oilValue) {
    bool critical = isCritical(0,0,0,false,true,oilValue);
    bool show = getBlinkState(critical);
    int col = oilValue ? COLOR_WHITE : COLOR_DARK;

    // Draw the icon only if blink is on
    if(show) {
      drawOilIcon(cx, cy-6, col);
      // Outer circle (rim) always visible
      drawGaugeCircle(cx, cy, radius, true);
    }    


    // Label and value always visible
    drawGaugeLabel(cx, cy, radius, "OIL", true, (int)oilValue);
}

void drawCoolant(int cx, int cy, int radius, int value) {
    bool critical = isCritical(value, OPTIMUM_TEMP_MIN, OPTIMUM_TEMP_MAX, true);
    bool show = getBlinkState(critical);
    int col = show ? (critical ? COLOR_WHITE : COLOR_DARK) : COLOR_BLACK;

    if(show) {
        drawHalfPieFill(cx, cy, radius, value, 60, 120, col);
        int nx = cx + cos((-180 + float(value - 60)/(120-60)*180) * M_PI / 180.0) * (radius-5);
        int ny = cy + sin((-180 + float(value - 60)/(120-60)*180) * M_PI / 180.0) * (radius-5);
        drawLine(cx, cy, nx, ny, COLOR_WHITE);
        drawGaugeCircle(cx, cy, radius, false);
    }
    drawGaugeLabel(cx, cy, radius, "COOL", false, value);
}

void drawFuel(int cx, int cy, int radius, int value) {
    bool critical = isCritical(value, 30, 100);
    bool show = getBlinkState(critical);
    int col = show ? (critical ? COLOR_WHITE : COLOR_DARK) : COLOR_BLACK;

    if(show) {
        drawHalfPieFill(cx, cy, radius, value, 0, 100, col);
        int nx = cx + cos((-180 + float(value - 0)/(100-0)*180) * M_PI / 180.0) * (radius-5);
        int ny = cy + sin((-180 + float(value - 0)/(100-0)*180) * M_PI / 180.0) * (radius-5);
        drawLine(cx, cy, nx, ny, COLOR_WHITE);
        drawGaugeCircle(cx, cy, radius, false);
    }
    drawGaugeLabel(cx, cy, radius, "FUEL", false, value);
}

void drawSpeed(int x, int y, int value) {
    drawBigNumber(x, y-30, value, 4, COLOR_WHITE);
    graphics.setTextColor(COLOR_WHITE);
    graphics.setCursor(x-(4*font.xres)/2, y+30);
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

// ------------------ Dashboard ------------------
void drawDashboard() {
    graphics.begin(COLOR_BLACK);
    int xres = graphics.xres;
    int yres = graphics.yres;

    int bottomBarTopY = yres*0.60;
    int thirdWidth = xres/3;
    int gaugeY = bottomBarTopY+40;

    // Draw metrics
    drawSpeed(xres/2, bottomBarTopY/2, speed);
    drawCoolant(thirdWidth+thirdWidth/2, gaugeY, 40, coolantTemp);
    drawFuel(xres-thirdWidth/2, gaugeY, 30, fuelLevel);
    drawOil(10+30, gaugeY, 30, oilLow);

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
}

void loop() {
    long t = millis();
    speed = 110 + sin(t*0.001)*20;
    coolantTemp = 95 + sin(t*0.0005)*15;
    fuelLevel = 50 - (t/2000)%50;
    oilLow = ((t/5000)%4 == 0); // simulate LOW oil occasionally

    drawDashboard();
    composite.sendFrameHalfResolution(&graphics.frame);
}
