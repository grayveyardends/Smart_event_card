#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#define WIFI_SSID   "POCO M6 Plus 5G"
#define WIFI_PASS   "wifi123456"

#define PIN_UP      0
#define PIN_DOWN    1
#define PIN_LEFT    2
#define PIN_RIGHT   3
#define PIN_ACTION  10
#define OLED_SDA    7
#define OLED_SCL    6

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);

struct Btn { int pin; bool pressed; bool held; bool prev; unsigned long last; };
Btn B[5] = {
    {PIN_UP,    false, false, false, 0},
    {PIN_DOWN,  false, false, false, 0},
    {PIN_LEFT,  false, false, false, 0},
    {PIN_RIGHT, false, false, false, 0},
    {PIN_ACTION,false, false, false, 0},
};

#define bUP  0
#define bDN  1
#define bLT  2
#define bRT  3
#define bAC  4
#define DEBOUNCE 110

void pollButtons() {
    unsigned long now = millis();
    for (int i = 0; i < 5; i++) {
        bool raw = (digitalRead(B[i].pin) == LOW);
        B[i].held    = raw;
        B[i].pressed = false;
        if (raw && !B[i].prev && now - B[i].last > DEBOUNCE) {
            B[i].pressed  = true;
            B[i].last     = now;
        }
        B[i].prev = raw;
    }
}

enum View { V_BOOT, V_MENU, V_ABOUT, V_PET, V_DINO, V_GAMEOVER, V_SOON };
View gView = V_BOOT;

bool needsRedraw = true;
void switchView(View v) { gView = v; needsRedraw = true; }

void drawBar(int x, int y, int w, int h, int pct, int r=2) {
    u8g2.drawRFrame(x, y, w, h, r);
    int fill = (w-2) * pct / 100;
    if (fill > 0) u8g2.drawBox(x+1, y+1, fill, h-2);
}

void drawHeader(const char* title, bool wifiIcon = false) {
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, 0, 128, 12);
    u8g2.setDrawColor(0);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(3, 9, title);
    if (wifiIcon) {
        bool wf = WiFi.status() == WL_CONNECTED;
        u8g2.drawStr(110, 9, wf ? "W" : "--");
    }
    u8g2.setDrawColor(1);
}

void drawFooter(const char* hint) {
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.setDrawColor(1);
    u8g2.drawStr(1, 63, hint);
}

unsigned long bootStart;

void renderBoot() {
    unsigned long t = millis() - bootStart;
    u8g2.clearBuffer();

    if (t < 400) {
        for (int i = 0; i < 3; i++) {
            int gy = random(0, 64);
            int gw = random(20, 80);
            int gx = random(0, 128 - gw);
            u8g2.drawBox(gx, gy, gw, 2);
        }
    }

    u8g2.setFont(u8g2_font_logisoso16_tf);
    u8g2.drawStr(12, 30, "FAZ");

    if (t > 300) {
        u8g2.setFont(u8g2_font_5x7_tf);
        u8g2.drawStr(28, 42, "v1.0  BLE MESH");
    }

    int pct = min((int)(t * 100 / 2600), 100);
    u8g2.drawRFrame(14, 47, 100, 7, 2);
    int fill = 98 * pct / 100;
    if (fill > 0) u8g2.drawBox(15, 48, fill, 5);

    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(14, 62, "Loading");
    int dots = (t / 350) % 4;
    for (int i = 0; i < dots; i++) {
        u8g2.drawStr(50 + i*5, 62, ".");
    }

    u8g2.sendBuffer();

    if (t > 2800) switchView(V_MENU);
}

const char* menuLabels[] = { "Events & Tracks", "Future Pet", "Dino Run", "About", "Reboot" };
const int   MENU_CNT = 5;
int  menuCursor = 0;
float menuAnimY  = 0;

static const uint8_t ICO_EVENTS[] PROGMEM = {0x1F,0x11,0x11,0x1D,0x11,0x11,0x1F,0x00};
static const uint8_t ICO_PET[]    PROGMEM = {0x00,0x0A,0x1F,0x1F,0x0E,0x04,0x00,0x00};
static const uint8_t ICO_DINO[]   PROGMEM = {0x0E,0x0F,0x0E,0x1E,0x1B,0x1B,0x12,0x00};
static const uint8_t ICO_INFO[]   PROGMEM = {0x04,0x00,0x0C,0x04,0x04,0x04,0x0E,0x00};
static const uint8_t ICO_POWER[]  PROGMEM = {0x04,0x0E,0x1B,0x11,0x1B,0x0E,0x04,0x00};
const uint8_t* menuIcons[5] = { ICO_EVENTS, ICO_PET, ICO_DINO, ICO_INFO, ICO_POWER };

void renderMenu() {
    float targetY = 13 + menuCursor * 11;
    menuAnimY += (targetY - menuAnimY) * 0.25f;

    u8g2.clearBuffer();
    drawHeader("ESP  BADGE", true);

    int cy = (int)menuAnimY;
    u8g2.drawRBox(0, cy, 128, 11, 2);

    for (int i = 0; i < MENU_CNT; i++) {
        int y = 13 + i * 11;
        bool sel = (i == menuCursor);
        u8g2.setDrawColor(sel ? 0 : 1);
        u8g2.drawXBMP(3, y+1, 5, 8, menuIcons[i]);
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(12, y + 8, menuLabels[i]);
        u8g2.setDrawColor(1);
    }

    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(108, 63, "[>] Open");

    u8g2.sendBuffer();
}

void handleMenu() {
    if (B[bUP].pressed)  { menuCursor = (menuCursor>0)?menuCursor-1:MENU_CNT-1; needsRedraw=true; }
    if (B[bDN].pressed)  { menuCursor = (menuCursor+1)%MENU_CNT; needsRedraw=true; }
    if (B[bRT].pressed || B[bAC].pressed) {
        switch (menuCursor) {
            case 0: switchView(V_SOON); break;
            case 1: switchView(V_PET);  break;
            case 2: switchView(V_DINO); break;
            case 3: switchView(V_ABOUT); break;
            case 4: ESP.restart(); break;
        }
    }
}

static const char* aboutText[] = {
    "  MISSION: BLE MESH ",
    "--------------------",
    "A mesh-enabled badge",
    "that tracks coverage",
    "& connects peers who",
    "share interests.",
    "",
    "No central hub.",
    "Just badges talking.",
    "",
    "Hardware:",
    " - ESP32-C3 Mini",
    " - SSD1306 128x64",
    " - 5 GPIO Buttons",
    "",
    "Built with love",
    "on PlatformIO :)",
    "",
    "[L] Back"
};
const int ABOUT_LINES = 19;
int aboutScroll = 0;

void renderAbout() {
    u8g2.clearBuffer();
    drawHeader("About & Support");

    const int VISIBLE = 6;
    u8g2.setFont(u8g2_font_5x7_tf);
    for (int i = 0; i < VISIBLE; i++) {
        int li = aboutScroll + i;
        if (li >= ABOUT_LINES) break;
        int y = 13 + i * 8;
        if (aboutText[li][0] == ' ' && aboutText[li][1] == ' ') {
            u8g2.drawBox(0, y - 1, 122, 8);
            u8g2.setDrawColor(0);
        }
        u8g2.drawStr(2, y + 6, aboutText[li]);
        u8g2.setDrawColor(1);
    }

    if (ABOUT_LINES > VISIBLE) {
        int sbH = 50 * VISIBLE / ABOUT_LINES;
        int sbY = 13 + (50 - sbH) * aboutScroll / (ABOUT_LINES - VISIBLE);
        u8g2.drawVLine(126, 13, 50);
        u8g2.drawBox(125, sbY, 3, sbH);
    }

    drawFooter("[^v] Scroll  [L] Back");
    u8g2.sendBuffer();
}

void handleAbout() {
    if (B[bUP].pressed && aboutScroll > 0)                   { aboutScroll--; needsRedraw=true; }
    if (B[bDN].pressed && aboutScroll < ABOUT_LINES-6)       { aboutScroll++; needsRedraw=true; }
    if (B[bLT].pressed) { aboutScroll=0; switchView(V_MENU); }
}

int   petHunger  = 80;
int   petHappy   = 75;
int   petFrame   = 0;
unsigned long petTick = 0;

void drawPetSprite(int x, int y, int frame, bool hungry) {
    u8g2.drawRBox(x+1, y+3, 14, 11, 3);
    u8g2.drawRBox(x+3, y-5, 11, 9, 3);
    u8g2.drawTriangle(x+3,y-5, x+1,y-10, x+5,y-5);
    u8g2.drawTriangle(x+13,y-5, x+11,y-10, x+15,y-5);

    bool blink = (frame % 60 > 58);
    u8g2.setDrawColor(0);
    if (blink) {
        u8g2.drawHLine(x+5, y-2, 2);
        u8g2.drawHLine(x+10, y-2, 2);
    } else {
        u8g2.drawDisc(x+6, y-2, 2);
        u8g2.drawDisc(x+11, y-2, 2);
        u8g2.setDrawColor(1);
        u8g2.drawPixel(x+5, y-3);
        u8g2.drawPixel(x+10, y-3);
        u8g2.setDrawColor(0);
    }

    if (hungry) {
        u8g2.drawCircle(x+9, y+4, 3, U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT);
    } else {
        u8g2.drawCircle(x+9, y+5, 3, U8G2_DRAW_LOWER_RIGHT|U8G2_DRAW_LOWER_LEFT);
    }
    u8g2.setDrawColor(1);

    int legPhase = frame % 8;
    u8g2.drawLine(x+4,  y+14, x+4 + (legPhase<4?-2:2), y+19);
    u8g2.drawLine(x+12, y+14, x+12 + (legPhase<4?2:-2), y+19);
    u8g2.drawHLine(x+2 + (legPhase<4?-2:2), y+19, 3);
    u8g2.drawHLine(x+11 + (legPhase<4?2:-2), y+19, 3);

    int tw = (frame % 10 < 5) ? 3 : -3;
    u8g2.drawLine(x+15, y+8, x+20, y+8+tw);
    u8g2.drawLine(x+20, y+8+tw, x+22, y+5+tw);
}

void renderPet() {
    if (millis() - petTick > 100) {
        petTick = millis();
        petFrame++;
        if (petFrame % 100 == 0 && petHunger > 0)  petHunger--;
        if (petFrame % 150 == 0 && petHappy > 0)   petHappy--;
    }

    bool hungry = petHunger < 30;

    u8g2.clearBuffer();
    drawHeader("FUTURE PET");

    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(0, 22, "HUN");
    drawBar(0, 23, 44, 5, petHunger);
    u8g2.drawStr(0, 36, "HAP");
    drawBar(0, 37, 44, 5, petHappy);
    u8g2.drawStr(0, 50, "LVL");
    drawBar(0, 51, 44, 5, 60);

    drawPetSprite(72, 25, petFrame, hungry);

    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawRFrame(70, 48, 40, 11, 2);
    u8g2.drawStr(75, 57, "BEEPY");

    drawFooter("[A] Feed  [L] Back");
    u8g2.sendBuffer();
}

void handlePet() {
    if (B[bAC].pressed) {
        petHunger = min(100, petHunger + 35);
        petHappy  = min(100, petHappy  + 15);
        needsRedraw = true;
    }
    if (B[bLT].pressed) switchView(V_MENU);
}

float dinoY        = 45;
float dinoVel      = 0;
bool  dinoJump     = false;
bool  dinoDuck     = false;
const float GRAVITY     = 0.75f;
const float JUMP_FORCE  = -10.0f;
const float GROUND_Y    = 46.0f;

float obsX         = 128;
int   obsType      = 0;
float obsY         = 45;

int   dinoScore    = 0;
int   hiScore      = 0;
float gameSpeed    = 3.5f;
int   gFrame       = 0;
bool  gameRunning  = false;

unsigned long lastFrameMs = 0;
const unsigned long FRAME_MS = 16;

int gnd[8] = {10,30,50,70,90,110,20,60};

void dinoReset() {
    dinoY = GROUND_Y; dinoVel = 0; dinoJump = false; dinoDuck = false;
    obsX = 140; dinoScore = 0; gameSpeed = 3.5f; gFrame = 0; gameRunning = true;
    lastFrameMs = millis();
}

void drawDinoSprite(int x, int y, bool jump, bool duck, int f) {
    if (duck) {
        u8g2.drawRBox(x, y+4, 16, 7, 1);
        u8g2.setDrawColor(0); u8g2.drawPixel(x+14, y+5); u8g2.setDrawColor(1);
        u8g2.drawLine(x+3, y+11, x+1, y+15);
        u8g2.drawLine(x+10, y+11, x+12, y+15);
        return;
    }
    u8g2.drawRBox(x+1, y+2, 11, 9, 1);
    u8g2.drawRBox(x+4, y-5, 10, 8, 1);
    u8g2.setDrawColor(0); u8g2.drawPixel(x+12, y-2); u8g2.setDrawColor(1);
    u8g2.drawBox(x+13, y, 3, 2);
    u8g2.drawLine(x+1, y+5, x-2, y+9);
    if (!jump) {
        int lp = f % 8;
        u8g2.drawLine(x+4,  y+11, x+4  + (lp<4?-2:2), y+17);
        u8g2.drawLine(x+9,  y+11, x+9  + (lp<4?2:-2), y+17);
        u8g2.drawHLine(x+2+(lp<4?-2:2), y+17, 3);
        u8g2.drawHLine(x+8+(lp<4?2:-2), y+17, 3);
    } else {
        u8g2.drawLine(x+4, y+11, x+2,  y+16);
        u8g2.drawLine(x+9, y+11, x+11, y+16);
    }
}

void drawObstacle(int x, int y, int type, int f) {
    if (type == 0) {
        u8g2.drawBox(x+3, y-4, 4, 12);
        u8g2.drawBox(x,   y+2, 10, 3);
        u8g2.drawBox(x,   y-1, 3, 6);
        u8g2.drawBox(x+7, y,   3, 5);
    } else if (type == 1) {
        u8g2.drawBox(x+3, y-10, 4, 18);
        u8g2.drawBox(x,   y+2,  10, 3);
        u8g2.drawBox(x,   y-4,  3,  8);
        u8g2.drawBox(x+7, y,    3,  5);
    } else {
        int wingUp = (f % 6 < 3);
        u8g2.drawLine(x,   y + (wingUp? 0:4), x+5, y + (wingUp?-4:0));
        u8g2.drawLine(x+7, y + (wingUp?-4:0), x+12,y + (wingUp? 0:4));
        u8g2.drawRBox(x+4, y-2, 6, 5, 1);
        u8g2.drawLine(x+10, y, x+13, y+1);
    }
}

void runDino() {
    pollButtons();

    if (B[bLT].pressed) { switchView(V_MENU); return; }

    if ((B[bUP].pressed || B[bAC].pressed) && !dinoJump && !dinoDuck) {
        dinoJump = true;
        dinoVel  = JUMP_FORCE;
    }
    dinoDuck = (digitalRead(PIN_DOWN) == LOW) && !dinoJump;

    unsigned long now = millis();
    if (now - lastFrameMs < FRAME_MS) return;
    lastFrameMs = now;
    gFrame++;

    if (dinoJump) {
        dinoY   += dinoVel;
        dinoVel += GRAVITY;
        if (dinoY >= GROUND_Y) {
            dinoY = GROUND_Y; dinoVel = 0; dinoJump = false;
        }
    }

    gameSpeed = min(3.5f + dinoScore * 0.25f, 13.0f);
    obsX -= gameSpeed;
    if (obsX < -15) {
        dinoScore++;
        obsX    = 128 + random(10, 40);
        obsType = random(0, dinoScore > 6 ? 3 : 2);
        obsY    = (obsType == 2) ? 32.0f : GROUND_Y;
    }

    for (int i = 0; i < 8; i++) {
        gnd[i] -= (int)(gameSpeed * 0.5f);
        if (gnd[i] < 0) gnd[i] = 128 + random(5, 25);
    }

    int dL = 14, dR = 27;
    int dT = (int)dinoY - (dinoDuck ? 0 : 5);
    int dB = (int)dinoY + 18;
    int oL = (int)obsX + 2,  oR = (int)obsX + 9;
    int oT = (int)obsY - (obsType == 1 ? 10 : 4);
    int oB = (int)obsY + 11;
    if (dR > oL && dL < oR && dB > oT && dT < oB) {
        if (dinoScore > hiScore) hiScore = dinoScore;
        gameRunning = false;
        switchView(V_GAMEOVER);
        return;
    }

    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_5x7_tf);
    char sb[20];
    sprintf(sb, "HI:%04d", hiScore);  u8g2.drawStr(0,  8, sb);
    sprintf(sb, "%04d", dinoScore);   u8g2.drawStr(96, 8, sb);

    int cl1 = (gFrame % 200);
    u8g2.drawRFrame((128 - cl1 + 256) % 140, 14, 22, 8, 3);
    u8g2.drawRFrame((128 - cl1 + 70 + 256) % 140, 17, 16, 6, 2);

    u8g2.drawLine(0, 58, 128, 58);
    for (int i = 0; i < 8; i++) {
        u8g2.drawPixel(gnd[i], 59);
        u8g2.drawHLine(gnd[i]+2, 60, 2);
    }

    drawDinoSprite(10, (int)dinoY, dinoJump, dinoDuck, gFrame);
    drawObstacle((int)obsX, (int)obsY, obsType, gFrame);

    if (gameSpeed > 8.0f) {
        u8g2.setFont(u8g2_font_4x6_tf);
        u8g2.drawStr(0, 63, (gFrame % 4 < 2) ? ">>FAST!" : ">> FAST");
    }

    u8g2.sendBuffer();
}

void renderGameOver() {
    u8g2.clearBuffer();
    if ((millis() / 350) % 2 == 0) {
        u8g2.setFont(u8g2_font_logisoso16_tf);
        u8g2.drawStr(10, 30, "GAME OVER");
    }
    u8g2.setFont(u8g2_font_6x10_tf);
    char sb[24];
    sprintf(sb, "Score : %d", dinoScore); u8g2.drawStr(24, 43, sb);
    sprintf(sb, "Best  : %d", hiScore);   u8g2.drawStr(24, 54, sb);
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(10, 63, "[A] Retry    [L] Menu");
    u8g2.sendBuffer();
}

void handleGameOver() {
    if (B[bAC].pressed || B[bRT].pressed) { dinoReset(); switchView(V_DINO); }
    if (B[bLT].pressed) switchView(V_MENU);
}

void renderSoon() {
    u8g2.clearBuffer();
    drawHeader(menuLabels[menuCursor]);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(18, 35, "Coming Soon...");
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(30, 50, "Check for updates");
    drawFooter("[L] Back");
    u8g2.sendBuffer();
}

void setup() {
    Serial.begin(115200);
    u8g2.begin();
    u8g2.setBusClock(400000);

    pinMode(PIN_UP,    INPUT_PULLUP);
    pinMode(PIN_DOWN,  INPUT_PULLUP);
    pinMode(PIN_LEFT,  INPUT_PULLUP);
    pinMode(PIN_RIGHT, INPUT_PULLUP);
    pinMode(PIN_ACTION,INPUT_PULLUP);

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    bootStart = millis();
    petTick   = millis();
}

void loop() {
    pollButtons();

    switch (gView) {
        case V_BOOT:
            renderBoot();
            break;
        case V_MENU:
            renderMenu();
            handleMenu();
            break;
        case V_ABOUT:
            if (needsRedraw) { renderAbout(); needsRedraw = false; }
            handleAbout();
            break;
        case V_PET:
            renderPet();
            handlePet();
            break;
        case V_DINO:
            if (gameRunning) runDino();
            break;
        case V_GAMEOVER:
            renderGameOver();
            handleGameOver();
            break;
        case V_SOON:
            if (needsRedraw) { renderSoon(); needsRedraw = false; }
            if (B[bLT].pressed) switchView(V_MENU);
            break;
    }
}
