/*
 * Daffodil International University - Campus Scene (FINAL)
 *
 * CONTROLS
 *   D / N        - Day / Night (animated transition)
 *   R            - Toggle Rain
 *   Arrow keys   - Move first car left / right
 *   ESC          - Quit
 */

#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/glut.h>
#include <cmath>
#include <cstring>
#include <string>
#include <cstdlib>

//constant
static const int   WIN_W  = 1100;
static const int   WIN_H  = 680;
static const float PI     = 3.14159265f;

//lerp
static float lerp(float a, float b, float t){ return a + (b - a) * t; }


//sky transition
static float g_skyT      = 1.0f;   // 0 = night, 1 = day
static float g_skyTarget = 1.0f;
static const float SKY_SPEED = 0.004f;
static bool  g_isDay = true;

//rain
static bool g_rain = false;
struct Raindrop { float x, y, speed, length; };
static const int MAX_DROPS = 300;
static Raindrop g_drops[MAX_DROPS];

static void initRain()
{
    for(int i = 0; i < MAX_DROPS; ++i)
    {
        g_drops[i].x      = (float)(rand() % WIN_W);
        g_drops[i].y      = (float)(rand() % WIN_H);
        g_drops[i].speed  = 6.0f + (rand() % 60) * 0.1f;
        g_drops[i].length = 8.0f + (rand() % 10);
    }
}

//wind
static float g_windTime  = 0.0f;
static const float WIND_AMP   = 3.5f;
static const float WIND_SPEED = 0.018f;

//clouds
struct Cloud { float x, y, scale, speed; };
static Cloud g_clouds[] =
{
    { 150.0f, 620.0f, 1.00f, 0.40f },
    { 420.0f, 640.0f, 0.75f, 0.25f },
    { 680.0f, 610.0f, 0.90f, 0.55f },
    { 900.0f, 630.0f, 0.65f, 0.30f },
    {1050.0f, 645.0f, 0.55f, 0.18f },
};

//window flicker
static float g_flickerTimer = 0.0f;
// Per-window random brightness offsets
static float g_winFlicker[7][7];  // [row][col] for centre block
static float g_wingFlicker[6][3]; // for both wings

static void initFlicker()
{
    for(int r = 0; r < 7; ++r)
        for(int c = 0; c < 7; ++c)
            g_winFlicker[r][c] = (float)(rand() % 100) / 100.0f;
    for(int r = 0; r < 6; ++r)
        for(int c = 0; c < 3; ++c)
            g_wingFlicker[r][c] = (float)(rand() % 100) / 100.0f;
}

//flag
static float g_flagTime = 0.0f;


//multiple cars
struct Car {
    float x;       // centre x
    float speed;   // px/frame (positive = rightward)
    float r, g, b; // body colour
    bool  topLane; // true = moving right in top lane, false = left in bottom lane
};
static Car g_cars[] =
{
    //top lane
    {  200.0f,  1.8f, 0.85f, 0.10f, 0.10f, true  },
    {  650.0f,  2.2f, 0.10f, 0.45f, 0.85f, true  },
    // bottom lane
    {  850.0f, -1.6f, 0.15f, 0.65f, 0.20f, false },
    {  350.0f, -2.0f, 0.80f, 0.55f, 0.05f, false },
};
static const int NUM_CARS = 4;

//manual control of car speed
static float g_carSpeed = 0.0f;

//walking person
struct Walker {
    float x;
    float speed;  // px/frame
    float phase;  // walk cycle offset
    float r, g, b;
    float y;      // baseline y
};
static Walker g_walkers[] =
{
    { 200.0f,  0.6f, 0.0f,  0.05f, 0.05f, 0.05f, 175.0f },
    { 700.0f, -0.5f, 1.5f,  0.20f, 0.15f, 0.10f, 175.0f },
    { 400.0f,  0.4f, 3.0f,  0.05f, 0.05f, 0.15f, 170.0f },
    { 900.0f, -0.7f, 0.8f,  0.15f, 0.05f, 0.05f, 172.0f },
};
static const int NUM_WALKERS = 4;
static float g_walkTime = 0.0f;

//birds
struct Bird {
    float x, y;
    float speed;
    float flapPhase;
    float wavePhase;
};
static Bird g_birds[] =
{
    {  100.0f, 580.0f, 0.8f, 0.0f, 0.0f  },
    {  250.0f, 600.0f, 1.1f, 1.2f, 0.5f  },
    {  -50.0f, 560.0f, 0.6f, 2.4f, 1.0f  },
    {  400.0f, 590.0f, 0.9f, 0.6f, 2.0f  },
    {  700.0f, 610.0f, 1.3f, 1.8f, 1.5f  },
};
static const int NUM_BIRDS = 5;
static float g_birdTime = 0.0f;

//fountain
static float g_fountainTime = 0.0f;

//primitive helpers
static void setColor3(float r, float g, float b){ glColor3f(r,g,b); }
static void setColor4(float r, float g, float b, float a){ glColor4f(r,g,b,a); }

static void fillRect(float x, float y, float w, float h)
{
    glBegin(GL_QUADS);
    glVertex2f(x,   y);   glVertex2f(x+w, y);
    glVertex2f(x+w, y+h); glVertex2f(x,   y+h);
    glEnd();
}

static void fillCircle(float cx, float cy, float r, int segs = 40)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for(int i = 0; i <= segs; ++i)
    {
        float a = 2.0f*PI*i/segs;
        glVertex2f(cx + r*cosf(a), cy + r*sinf(a));
    }
    glEnd();
}

static void strokeCircle(float cx, float cy, float r, int segs = 40, float lw = 1.5f)
{
    glLineWidth(lw);
    glBegin(GL_LINE_LOOP);
    for(int i = 0; i < segs; ++i)
    {
        float a = 2.0f*PI*i/segs;
        glVertex2f(cx + r*cosf(a), cy + r*sinf(a));
    }
    glEnd();
    glLineWidth(1.0f);
}

static void fillEllipse(float cx, float cy, float rx, float ry, int segs = 30)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for(int i = 0; i <= segs; ++i)
    {
        float a = 2.0f*PI*i/segs;
        glVertex2f(cx + rx*cosf(a), cy + ry*sinf(a));
    }
    glEnd();
}

static void drawText(float x, float y, const char* s, void* font = GLUT_BITMAP_HELVETICA_18)
{
    glRasterPos2f(x, y);
    for(const char* c = s; *c; ++c) glutBitmapCharacter(font, *c);
}

//DDA for road's lane divider
static void drawLineDDA(float x0, float y0, float x1, float y1)
{
    float dx = x1 - x0;
    float dy = y1 - y0;
    float steps = (fabsf(dx) > fabsf(dy)) ? fabsf(dx) : fabsf(dy);

    if(steps == 0) return;

    float xInc = dx / steps;
    float yInc = dy / steps;
    float x = x0;
    float y = y0;

    // Output vertices - caller wraps with glBegin/glEnd
    for(int i = 0; i <= (int)steps; ++i)
    {
        glVertex2f(x, y);
        x += xInc;
        y += yInc;
    }
}

//bresenham to draw the ramp
static void drawLineBresenham(int x0, int y0, int x1, int y1)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    int x = x0;
    int y = y0;

    glBegin(GL_POINTS);
    while(true)
    {
        glVertex2f((float)x, (float)y);

        if(x == x1 && y == y1) break;

        int e2 = 2 * err;
        if(e2 > -dy)
        {
            err -= dy;
            x += sx;
        }
        if(e2 < dx)
        {
            err += dx;
            y += sy;
        }
    }
    glEnd();
}

//sky
static void drawSky()
{
    float t = g_skyT;

    float tr = lerp(0.04f, 0.53f, t);
    float tg = lerp(0.04f, 0.81f, t);
    float tb = lerp(0.14f, 0.98f, t);

    float br = lerp(0.06f, 0.78f, t);
    float bg = lerp(0.06f, 0.93f, t);
    float bb = lerp(0.18f, 1.00f, t);

    float gr = lerp(0.12f, 0.90f, t);
    float gg = lerp(0.10f, 0.85f, t);
    float gb = lerp(0.20f, 0.72f, t);

    // Sunrise/sunset blush
    float blush = (t < 0.5f) ? (t * 2.0f) : (2.0f - t * 2.0f);
    blush *= 0.35f;

    glBegin(GL_QUADS);
    glColor3f(tr, tg, tb);
    glVertex2f(0, WIN_H);
    glColor3f(tr, tg, tb);
    glVertex2f(WIN_W, WIN_H);
    glColor3f(br + blush, bg * (1.0f - blush*0.3f), bb * (1.0f - blush*0.5f));
    glVertex2f(WIN_W, 200);
    glColor3f(br + blush, bg * (1.0f - blush*0.3f), bb * (1.0f - blush*0.5f));
    glVertex2f(0, 200);
    glEnd();

    glColor3f(gr, gg, gb);
    fillRect(0, 0, WIN_W, 200);
}

//stars
static void drawStars()
{
    if(g_isDay) return;
    setColor3(1,1,1);
    unsigned int s = 0x1234abcd;
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for(int i = 0; i < 120; ++i)
    {
        s = s*1664525u + 1013904223u;
        float sx = (s & 0xFFFF) / 65535.0f * WIN_W;
        s = s*1664525u + 1013904223u;
        float sy = (s & 0xFFFF) / 65535.0f * (WIN_H - 250) + 250;
        glVertex2f(sx, sy);
    }
    glEnd();
    glPointSize(1.0f);
}

//son/moon
static void drawSun()
{
    float cx = 980, cy = 610;
    if(g_isDay)
    {
        setColor3(1.0f,0.85f,0.0f);
        glLineWidth(2.5f);
        glBegin(GL_LINES);
        for(int i = 0; i < 12; ++i)
        {
            float a = 2*PI*i/12;
            glVertex2f(cx + 30*cosf(a), cy + 30*sinf(a));
            glVertex2f(cx + 46*cosf(a), cy + 46*sinf(a));
        }
        glEnd();
        glLineWidth(1);
        setColor3(1.0f,0.90f,0.05f);
        fillCircle(cx, cy, 26);
        setColor3(1.0f,0.75f,0.0f);
        strokeCircle(cx, cy, 26, 40, 1.5f);
    }
    else
    {
        setColor3(0.95f,0.95f,0.80f);
        fillCircle(cx, cy, 22);
        setColor3(0.06f,0.06f,0.18f);
        fillCircle(cx+12, cy+6, 18);
    }
}

//clouds
static void drawCloud(float cx, float cy, float scale)
{
    if(!g_isDay) setColor4(0.3f,0.3f,0.4f,0.5f);
    else         setColor3(0.97f,0.97f,1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    fillEllipse(cx,          cy,       50*scale, 22*scale);
    fillEllipse(cx-30*scale, cy-6*scale,  32*scale, 20*scale);
    fillEllipse(cx+28*scale, cy-4*scale,  34*scale, 18*scale);
    fillEllipse(cx-10*scale, cy+6*scale,  42*scale, 14*scale);
    glDisable(GL_BLEND);
}

//rain
static void drawRain()
{
    if(!g_rain) return;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for(int i = 0; i < MAX_DROPS; ++i)
    {
        float x2 = g_drops[i].x + g_drops[i].length * 0.25f;
        float y2 = g_drops[i].y - g_drops[i].length;
        if(g_isDay) glColor4f(0.3f, 0.4f, 0.8f, 0.55f);
        else        glColor4f(0.5f, 0.6f, 0.9f, 0.45f);
        glVertex2f(g_drops[i].x, g_drops[i].y);
        glVertex2f(x2, y2);
    }
    glEnd();
    glDisable(GL_BLEND);
}

//distanced city
static void drawCitySilhouette()
{
    float col = g_isDay ? 0.65f : 0.15f;
    setColor3(col, col, col+0.03f);
    struct { float x, y, w, h; } buildings[] =
    {
        {0,200,80,80},{70,200,40,110},{100,200,60,90},{155,200,30,70},
        {780,200,55,85},{825,200,45,100},{860,200,65,75},
        {910,200,35,90},{935,200,55,60},{985,200,50,80},{1030,200,70,100},
    };
    for(auto& b : buildings) fillRect(b.x, b.y, b.w, b.h);
}

//road
static void drawRoad()
{
    setColor3(0.30f,0.30f,0.32f);
    fillRect(0, 0, WIN_W, 155);

    setColor3(0.55f,0.55f,0.55f);
    fillRect(0, 152, WIN_W, 3);
    fillRect(0, 3,   WIN_W, 3);

    // Lane divider (middle of road) - drawn with DDA algorithm
    setColor3(0.90f,0.80f,0.10f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    drawLineDDA(0, 75.5f, WIN_W, 75.5f);  // DDA algorithm used here
    glEnd();
    glLineWidth(1.0f);

    setColor3(1,1,1);
    for(int x = 0; x < WIN_W; x += 60)
    {
        fillRect(x, 110, 36, 5); // top lane dashes
        fillRect(x, 42,  36, 5); // bottom lane dashes
    }

    setColor3(0.88f,0.82f,0.70f);
    fillRect(150, 155, 800, 200);
}

//tree with wind
static void drawTree(float bx, float by, float scale = 1.0f)
{
    float phase = bx * 0.07f;
    float sway  = WIND_AMP * scale * sinf(g_windTime + phase);

    setColor3(0.42f,0.27f,0.10f);
    glBegin(GL_QUADS);
    glVertex2f(bx - 5*scale, by);
    glVertex2f(bx + 5*scale, by);
    glVertex2f(bx + 5*scale + sway*0.4f, by + 35*scale);
    glVertex2f(bx - 5*scale + sway*0.4f, by + 35*scale);
    glEnd();

    float cx = bx + sway;
    if(g_isDay) setColor3(0.20f,0.58f,0.15f);
    else        setColor3(0.10f,0.28f,0.08f);
    fillCircle(cx, by+35*scale+18*scale, 22*scale);

    if(g_isDay) setColor3(0.14f,0.48f,0.10f);
    else        setColor3(0.06f,0.20f,0.06f);
    fillCircle(cx-10*scale, by+35*scale+12*scale, 16*scale);
    fillCircle(cx+10*scale, by+35*scale+12*scale, 16*scale);
}

//pine tree
static void drawPineTree(float bx, float by, float scale = 1.0f)
{
    float phase = bx * 0.07f;
    float sway  = WIND_AMP * scale * 0.6f * sinf(g_windTime + phase);

    setColor3(0.42f,0.27f,0.10f);
    fillRect(bx-4*scale, by, 8*scale, 20*scale);

    if(g_isDay) setColor3(0.07f,0.38f,0.10f);
    else        setColor3(0.04f,0.18f,0.06f);

    glBegin(GL_TRIANGLES);
    glVertex2f(bx + sway*0.4f,           by+20*scale+50*scale);
    glVertex2f(bx-20*scale + sway*0.2f,  by+20*scale);
    glVertex2f(bx+20*scale + sway*0.2f,  by+20*scale);
    glEnd();
    glBegin(GL_TRIANGLES);
    glVertex2f(bx + sway,                by+20*scale+38*scale);
    glVertex2f(bx-16*scale + sway*0.5f,  by+20*scale+8*scale);
    glVertex2f(bx+16*scale + sway*0.5f,  by+20*scale+8*scale);
    glEnd();
}

//bench
static void drawBench(float x, float y)
{
    setColor3(0.45f,0.25f,0.10f);
    fillRect(x,    y+10, 50, 5);
    fillRect(x+5,  y,    5,  12);
    fillRect(x+40, y,    5,  12);
    fillRect(x,    y+18, 50, 4);
}

//lamp post
static void drawLampPost(float x, float by)
{
    setColor3(0.20f,0.20f,0.22f);
    fillRect(x-3, by, 6, 80);
    setColor3(0.22f,0.22f,0.24f);
    fillRect(x-3, by+78, 18, 4);

    if(g_isDay)
    {
        setColor3(0.80f,0.80f,0.50f);
    }
    else
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        setColor4(1.0f,0.95f,0.60f,0.35f);
        fillCircle(x+15, by+86, 20);
        glDisable(GL_BLEND);
        setColor3(1.0f,0.95f,0.60f);
    }
    fillCircle(x+15, by+86, 7);
}

//shrub
static void drawShrub(float cx, float cy)
{
    if(g_isDay) setColor3(0.18f,0.50f,0.12f);
    else        setColor3(0.08f,0.22f,0.06f);
    fillCircle(cx,   cy,   9);
    fillCircle(cx-8, cy-2, 7);
    fillCircle(cx+8, cy-2, 7);
    fillCircle(cx,   cy-8, 7);
}

//main building
static void drawBuilding()
{
    float wb = lerp(0.30f, 0.85f, g_skyT);

    //left wing
    setColor3(wb,wb,wb);
    fillRect(170, 280, 160, 270);

    for(int row = 0; row < 6; ++row)
    for(int col = 0; col < 3; ++col)
    {
        float wx = 185 + col*44;
        float wy = 300 + row*38;

        float flick = 1.0f;
        if(!g_isDay)
        {
            // flicker effect
            flick = 0.75f + 0.25f * sinf(g_flickerTimer * 3.0f + g_wingFlicker[row][col] * 20.0f);
            // rarely a window goes dark
            if(g_wingFlicker[row][col] < 0.08f &&
               sinf(g_flickerTimer * 0.7f + g_wingFlicker[row][col]*50.0f) < -0.8f)
                flick = 0.05f;
        }

        if(g_isDay) setColor3(0.45f,0.65f,0.85f);
        else        glColor3f(0.15f*flick, 0.20f*flick, 0.55f*flick);
        fillRect(wx, wy, 30, 24);

        if(row == 2 || row == 4)
        {
            setColor3(0.55f,0.32f,0.10f);
            fillRect(wx, wy-8, 30, 6);
        }
    }

    //center block
    setColor3(wb,wb,wb);
    fillRect(330, 250, 440, 300);

    for(int row = 0; row < 7; ++row)
    for(int col = 0; col < 7; ++col)
    {
        float wx = 345 + col*55;
        float wy = 265 + row*36;

        float flick = 1.0f;
        if(!g_isDay)
        {
            flick = 0.75f + 0.25f * sinf(g_flickerTimer * 2.5f + g_winFlicker[row][col] * 20.0f);
            if(g_winFlicker[row][col] < 0.10f &&
               sinf(g_flickerTimer * 0.5f + g_winFlicker[row][col]*50.0f) < -0.85f)
                flick = 0.05f;
        }

        if(col == 2 || col == 3 || col == 4)
        {
            if(g_isDay) setColor3(0.35f,0.60f,0.90f);
            else        glColor3f(0.10f*flick, 0.15f*flick, 0.60f*flick);
            fillRect(wx, wy, 42, 28);
        }
        else
        {
            if(g_isDay) setColor3(0.40f,0.62f,0.82f);
            else        glColor3f(0.12f*flick, 0.18f*flick, 0.50f*flick);
            fillRect(wx, wy, 35, 24);
            if(row == 1 || row == 3 || row == 5)
            {
                setColor3(0.55f,0.32f,0.10f);
                fillRect(wx, wy-6, 35, 5);
            }
        }
    }

    //right wing
    setColor3(wb,wb,wb);
    fillRect(770, 270, 170, 280);

    for(int row = 0; row < 6; ++row)
    for(int col = 0; col < 3; ++col)
    {
        float wx = 785 + col*44;
        float wy = 285 + row*38;

        float flick = 1.0f;
        if(!g_isDay)
        {
            flick = 0.75f + 0.25f * sinf(g_flickerTimer * 3.5f + g_wingFlicker[row][col] * 22.0f);
            if(g_wingFlicker[row][col] < 0.06f &&
               sinf(g_flickerTimer * 0.9f + g_wingFlicker[row][col]*60.0f) < -0.9f)
                flick = 0.05f;
        }

        if(g_isDay) setColor3(0.45f,0.65f,0.85f);
        else        glColor3f(0.15f*flick, 0.20f*flick, 0.55f*flick);
        fillRect(wx, wy, 30, 24);

        if(row == 2 || row == 4)
        {
            setColor3(0.55f,0.32f,0.10f);
            fillRect(wx, wy-8, 30, 6);
        }
    }

    //entrance canopy
    setColor3(0.55f,0.32f,0.10f);
    fillRect(380, 390, 340, 18);
    setColor3(0.48f,0.27f,0.08f);
    fillRect(390, 395, 320, 8);

    //glass at entrance
    if(g_isDay) setColor3(0.40f,0.70f,0.95f);
    else        setColor3(0.10f,0.18f,0.50f);
    fillRect(410, 255, 280, 140);

    //reflection
    if(g_isDay)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        float refX = 415 + fmodf(g_flagTime * 18.0f, 260.0f);
        glBegin(GL_QUADS);
        glColor4f(1.0f,1.0f,1.0f, 0.0f);
        glVertex2f(refX,      255);
        glVertex2f(refX+4,    255);
        glColor4f(1.0f,1.0f,1.0f, 0.18f);
        glVertex2f(refX+14,   395);
        glVertex2f(refX+10,   395);
        glEnd();
        // Secondary faint reflection
        glBegin(GL_QUADS);
        glColor4f(1.0f,1.0f,1.0f, 0.0f);
        glVertex2f(refX+30,   255);
        glVertex2f(refX+34,   255);
        glColor4f(1.0f,1.0f,1.0f, 0.08f);
        glVertex2f(refX+44,   395);
        glVertex2f(refX+40,   395);
        glEnd();
        glDisable(GL_BLEND);
    }

    //column of entrance
    setColor3(wb-0.05f, wb-0.05f, wb-0.05f);
    for(int i = 0; i < 5; ++i) fillRect(415+i*56, 255, 12, 140);

    //door of entrance
    setColor3(0.15f,0.15f,0.17f);
    fillRect(510, 255, 80, 90);
    setColor3(0.60f,0.75f,0.90f);
    fillRect(514, 258, 34, 84);
    fillRect(552, 258, 34, 84);

    //steps
    setColor3(0.82f,0.78f,0.72f); fillRect(395, 248, 310, 12);
    setColor3(0.76f,0.72f,0.67f); fillRect(408, 260, 284, 12);
    setColor3(0.70f,0.66f,0.62f); fillRect(421, 272, 258, 12);
    setColor3(0.90f,0.87f,0.83f);
    fillRect(395, 259, 310, 2);
    fillRect(408, 271, 284, 2);
    fillRect(421, 283, 258, 2);

    //ramps - drawn with BRESENHAM algorithm
    setColor3(0.50f,0.30f,0.10f);
    glBegin(GL_QUADS);
    glVertex2f(395,248); glVertex2f(416,248);
    glVertex2f(375,205); glVertex2f(354,205);
    glEnd();
    glBegin(GL_QUADS);
    glVertex2f(684,248); glVertex2f(705,248);
    glVertex2f(746,205); glVertex2f(725,205);
    glEnd();

    setColor3(0.35f,0.20f,0.07f);
    glLineWidth(3.0f);
    glPointSize(3.0f);

    // Bresenham algorithm used here for ramp support lines
    drawLineBresenham(416, 260, 375, 215);  // Bresenham left ramp
    drawLineBresenham(684, 260, 725, 215);  // Bresenham right ramp

    glPointSize(1.0f);
    glLineWidth(1.0f);

    setColor3(0.78f,0.74f,0.69f);
    fillRect(354, 200, 62, 8);
    fillRect(725, 200, 62, 8);
}

//waving flags
static void drawFlag()
{
    // Flag pole on rooftop centre
    float poleX = 550.0f;
    float poleY = 548.0f;  // top of centre block
    float poleH = 55.0f;

    setColor3(0.25f,0.25f,0.27f);
    fillRect(poleX-1.5f, poleY, 3.0f, poleH);

    // Flag: 5 control points waving with sine
    float fw = 60.0f;  // flag width
    float fh = 24.0f;  // flag height
    float fy = poleY + poleH - fh;
    int segs = 12;

    // Green flag (DIU colour)
    glBegin(GL_TRIANGLE_STRIP);
    for(int i = 0; i <= segs; ++i)
    {
        float u   = (float)i / segs;
        float px  = poleX + u * fw;
        float wave = sinf(g_flagTime * 4.0f + u * 2.5f * PI) * 5.0f * u;

        if(g_isDay) glColor3f(0.05f, 0.60f, 0.20f);
        else        glColor3f(0.03f, 0.35f, 0.12f);
        glVertex2f(px, fy + fh*0.5f + wave + fh*0.5f);

        if(g_isDay) glColor3f(0.04f, 0.50f, 0.15f);
        else        glColor3f(0.02f, 0.28f, 0.09f);
        glVertex2f(px, fy + fh*0.5f + wave - fh*0.5f);
    }
    glEnd();

    // Red circle on flag
    float midU   = 0.45f;
    float midWave= sinf(g_flagTime * 4.0f + midU * 2.5f * PI) * 5.0f * midU;
    if(g_isDay) setColor3(0.85f, 0.10f, 0.10f);
    else        setColor3(0.55f, 0.07f, 0.07f);
    fillCircle(poleX + midU*fw, fy + fh*0.5f + midWave, 7.0f, 20);
}

//university sign
static void drawUniversitySign()
{
    setColor3(0.12f,0.22f,0.55f);
    fillRect(370, 535, 360, 36);

    glLineWidth(2.0f);
    setColor3(0.95f,0.78f,0.10f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(370,535); glVertex2f(730,535);
    glVertex2f(730,571); glVertex2f(370,571);
    glEnd();
    glLineWidth(1.0f);

    setColor3(1.0f,1.0f,1.0f);
    drawText(382, 549, "DAFFODIL INTERNATIONAL UNIVERSITY",
             GLUT_BITMAP_HELVETICA_12);
}

//drawing car
static void drawCarAt(float cx, float by, float r, float g, float b, bool facingRight)
{
    // Body
    glColor3f(r, g, b);
    fillRect(cx-62, by+10, 124, 38);

    // Cabin
    glColor3f(r*0.88f, g*0.88f, b*0.88f);
    fillRect(cx-42, by+26, 84, 26);

    // Windscreen
    setColor3(0.55f,0.78f,0.95f);
    fillRect(cx-36, by+28, 32, 20);
    fillRect(cx+6,  by+28, 28, 20);

    // Lights
    if(facingRight)
    {
        setColor3(0.95f,0.85f,0.15f);
        fillRect(cx+54, by+16, 12, 10);  // headlight
        setColor3(0.90f,0.05f,0.05f);
        fillRect(cx-66, by+16, 8, 10);   // tail
    }
    else
    {
        setColor3(0.95f,0.85f,0.15f);
        fillRect(cx-66, by+16, 12, 10);  // headlight
        setColor3(0.90f,0.05f,0.05f);
        fillRect(cx+54, by+16, 8, 10);   // tail
    }

    // Wheels
    setColor3(0.10f,0.10f,0.12f);
    fillCircle(cx-36, by+10, 16);
    fillCircle(cx+36, by+10, 16);
    setColor3(0.60f,0.60f,0.65f);
    fillCircle(cx-36, by+10, 8);
    fillCircle(cx+36, by+10, 8);

    // Night headlight glow
    if(!g_isDay)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        setColor4(1.0f,0.90f,0.50f,0.18f);
        if(facingRight) fillEllipse(cx+75, by+20, 45, 15);
        else            fillEllipse(cx-75, by+20, 45, 15);
        glDisable(GL_BLEND);
    }
}

//all cars
static void drawAllCars()
{
    for(int i = 0; i < NUM_CARS; ++i)
    {
        float y  = g_cars[i].topLane ? 80.0f : 35.0f;
        bool  fr = g_cars[i].speed > 0;
        drawCarAt(g_cars[i].x, y, g_cars[i].r, g_cars[i].g, g_cars[i].b, fr);
    }
}

//drawing walking person
static void drawWalkerAt(float cx, float by, float walkPhase, float r, float g2, float b2)
{
    float legSwing  = 8.0f * sinf(walkPhase);
    float armSwing  = 7.0f * sinf(walkPhase);
    float bobY      = fabsf(sinf(walkPhase)) * 1.5f;

    glColor3f(r, g2, b2);
    glLineWidth(2.5f);

    // Head
    fillCircle(cx, by + 46 + bobY, 7);

    glBegin(GL_LINES);
    // Body
    glVertex2f(cx, by+38+bobY); glVertex2f(cx, by+18+bobY);
    // Arms
    glVertex2f(cx, by+32+bobY); glVertex2f(cx - armSwing, by+22+bobY);
    glVertex2f(cx, by+32+bobY); glVertex2f(cx + armSwing, by+22+bobY);
    // Legs
    glVertex2f(cx, by+18+bobY); glVertex2f(cx - legSwing, by);
    glVertex2f(cx, by+18+bobY); glVertex2f(cx + legSwing, by);
    glEnd();
    glLineWidth(1.0f);
}

//walkers
static void drawAllWalkers()
{
    for(int i = 0; i < NUM_WALKERS; ++i)
    {
        float phase = g_walkTime * 5.0f * fabsf(g_walkers[i].speed) + g_walkers[i].phase;
        drawWalkerAt(g_walkers[i].x, g_walkers[i].y, phase,
                     g_walkers[i].r, g_walkers[i].g, g_walkers[i].b);
    }
}

//drawing birds
static void drawBirdAt(float cx, float cy, float flapPhase)
{
    float flapY = sinf(flapPhase) * 5.0f;  // wing tip dip

    if(g_isDay) setColor3(0.10f,0.08f,0.06f);
    else        setColor3(0.35f,0.35f,0.40f);

    glLineWidth(1.8f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(cx - 14, cy + flapY);
    glVertex2f(cx -  6, cy + 2);
    glVertex2f(cx,      cy);
    glVertex2f(cx +  6, cy + 2);
    glVertex2f(cx + 14, cy + flapY);
    glEnd();
    glLineWidth(1.0f);
}

//all birds
static void drawAllBirds()
{
    if(!g_isDay) return;  // birds rest at night
    for(int i = 0; i < NUM_BIRDS; ++i)
    {
        float flap = g_birdTime * 4.0f + g_birds[i].flapPhase;
        float waveY = sinf(g_birdTime * 1.5f + g_birds[i].wavePhase) * 8.0f;
        drawBirdAt(g_birds[i].x, g_birds[i].y + waveY, flap);
    }
}

//fountain
static void drawFountain()
{
    float fx = 550.0f;  // centre-x (in front of entrance)
    float fy = 193.0f;  // base y

    // Basin
    setColor3(0.70f,0.68f,0.65f);
    fillEllipse(fx, fy, 40, 10);
    setColor3(0.62f,0.60f,0.58f);
    glBegin(GL_LINE_LOOP);
    for(int i = 0; i < 30; ++i)
    {
        float a = 2*PI*i/30;
        glVertex2f(fx + 40*cosf(a), fy + 10*sinf(a));
    }
    glEnd();

    // Water in basin
    setColor3(0.35f,0.65f,0.90f);
    fillEllipse(fx, fy, 36, 8);

    // Pillar
    setColor3(0.75f,0.73f,0.70f);
    fillRect(fx-4, fy, 8, 22);

    // Animated water jets
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    int numJets = 6;
    for(int j = 0; j < numJets; ++j)
    {
        float angle = 2*PI*j/numJets + g_fountainTime * 0.3f;
        float jHeight = 28.0f + 5.0f * sinf(g_fountainTime * 2.0f + j);

        glLineWidth(1.5f);
        glBegin(GL_LINE_STRIP);
        for(int s = 0; s <= 12; ++s)
        {
            float u  = (float)s / 12.0f;
            float jx = fx + cosf(angle) * 25.0f * u;
            float jy = fy + 22 + jHeight * (1.0f - (2*u-1)*(2*u-1)) * sinf(g_fountainTime*3.0f + j*0.5f + 0.7f);
            float alpha = 0.7f * (1.0f - u * 0.5f);
            glColor4f(0.5f, 0.80f, 1.0f, alpha);
            glVertex2f(jx, jy);
        }
        glEnd();
    }
    glLineWidth(1.0f);
    glDisable(GL_BLEND);

    // Splash droplets
    glPointSize(2.5f);
    glBegin(GL_POINTS);
    for(int d = 0; d < 16; ++d)
    {
        float da = 2*PI*d/16 + g_fountainTime;
        float dr = 18.0f + 8.0f * sinf(g_fountainTime*2.5f + d);
        glColor4f(0.4f, 0.75f, 1.0f, 0.55f);
        glVertex2f(fx + cosf(da)*dr, fy + sinf(da)*dr*0.25f + 2);
    }
    glEnd();
    glPointSize(1.0f);
}

//campus furniture
static void drawCampusFurniture()
{
    if(g_isDay) setColor3(0.24f,0.60f,0.18f);
    else        setColor3(0.10f,0.28f,0.08f);
    fillEllipse(255, 178, 80, 22);

    if(g_isDay) setColor3(0.24f,0.60f,0.18f);
    else        setColor3(0.10f,0.28f,0.08f);
    fillEllipse(845, 175, 70, 20);

    drawBench(210, 183);
    drawBench(840, 183);

    drawLampPost(310, 155);
    drawLampPost(790, 155);

    for(int i = 0; i < 4; ++i) drawShrub(450+i*60, 210);

    drawTree(175, 168, 1.05f);
    drawTree(230, 165, 0.90f);
    drawTree(900, 165, 1.00f);
    drawTree(955, 168, 0.95f);
    drawTree(1010,165, 1.05f);

    drawPineTree(410, 165, 0.85f);
    drawPineTree(490, 163, 1.0f);
    drawPineTree(570, 162, 1.1f);
    drawPineTree(650, 163, 1.0f);
    drawPineTree(730, 165, 0.85f);
}

//night overlay
static void drawNightOverlay()
{
    float alpha = lerp(0.0f, 0.38f, 1.0f - g_skyT);
    if(alpha < 0.01f) return;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.02f,0.03f,0.12f, alpha);
    fillRect(0,0,WIN_W,WIN_H);
    glDisable(GL_BLEND);
}

//display
static void display()
{
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Environment
    drawSky();
    drawStars();
    drawCitySilhouette();
    drawSun();
    for(int i = 0; i < (int)(sizeof(g_clouds)/sizeof(g_clouds[0])); ++i)
        drawCloud(g_clouds[i].x, g_clouds[i].y, g_clouds[i].scale);

    // Campus
    drawRoad();
    drawFountain();
    drawBuilding();
    drawFlag();
    drawUniversitySign();
    drawCampusFurniture();

    // Life
    drawAllCars();
    drawAllWalkers();
    drawAllBirds();

    // Effects on top
    drawRain();
    drawNightOverlay();

    glDisable(GL_BLEND);
    glutSwapBuffers();
}

//timer all animation
static void timer(int)
{
    //sky
    if(g_skyT < g_skyTarget)
        g_skyT = std::min(g_skyT + SKY_SPEED, g_skyTarget);
    else if(g_skyT > g_skyTarget)
        g_skyT = std::max(g_skyT - SKY_SPEED, g_skyTarget);

    //wind
    g_windTime += WIND_SPEED;

    //clouds
    {
        int n = (int)(sizeof(g_clouds)/sizeof(g_clouds[0]));
        for(int i = 0; i < n; ++i)
        {
            g_clouds[i].x += g_clouds[i].speed;
            float halfW = 100.0f * g_clouds[i].scale;
            if(g_clouds[i].x - halfW > WIN_W) g_clouds[i].x = -halfW;
        }
    }

    //rain
    if(g_rain)
    {
        for(int i = 0; i < MAX_DROPS; ++i)
        {
            g_drops[i].y -= g_drops[i].speed;
            g_drops[i].x += 0.3f;
            if(g_drops[i].y < 0)
            {
                g_drops[i].y = WIN_H + 10.0f;
                g_drops[i].x = (float)(rand() % WIN_W);
            }
        }
    }

    //multiple car with car[0] have manual control
    g_cars[0].speed += g_carSpeed * 0.1f;  // player nudge blended in
    for(int i = 0; i < NUM_CARS; ++i)
    {
        g_cars[i].x += g_cars[i].speed;
        if(g_cars[i].speed > 0 && g_cars[i].x >  WIN_W + 120) g_cars[i].x = -120;
        if(g_cars[i].speed < 0 && g_cars[i].x < -120)         g_cars[i].x =  WIN_W + 120;
    }

    //walkers
    g_walkTime += 0.016f;
    for(int i = 0; i < NUM_WALKERS; ++i)
    {
        g_walkers[i].x += g_walkers[i].speed;
        if(g_walkers[i].x >  WIN_W + 30) g_walkers[i].x = -30;
        if(g_walkers[i].x < -30)         g_walkers[i].x =  WIN_W + 30;
    }

    //birds
    g_birdTime += 0.016f;
    for(int i = 0; i < NUM_BIRDS; ++i)
    {
        g_birds[i].x += g_birds[i].speed;
        if(g_birds[i].x > WIN_W + 50) g_birds[i].x = -50;
    }

    //flag
    g_flagTime += 0.016f;

    //flickerTime
    g_flickerTimer += 0.016f;

    //fountain
    g_fountainTime += 0.025f;

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

//input
static void keyboard(unsigned char key, int, int)
{
    switch(key)
    {
    case 'd': case 'D':
        g_isDay = true;
        g_skyTarget = 1.0f;
        break;
    case 'n': case 'N':
        g_isDay = false;
        g_skyTarget = 0.0f;
        break;
    case 'r': case 'R':
        g_rain = !g_rain;
        if(g_rain) initRain();
        break;
    case 27: exit(0); break;
    }
    glutPostRedisplay();
}

static void special(int key, int, int)
{
    switch(key)
    {
    case GLUT_KEY_LEFT:  g_carSpeed = -3.0f; g_cars[0].speed = -2.0f; break;
    case GLUT_KEY_RIGHT: g_carSpeed =  3.0f; g_cars[0].speed =  2.0f; break;
    }
    glutPostRedisplay();
}

static void specialUp(int key, int, int)
{
    (void)key;
    g_carSpeed = 0.0f;
    // restore auto speed for car[0]
    g_cars[0].speed = 1.8f;
    glutPostRedisplay();
}

static void reshape(int w, int h)
{
    if(h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

//main funciton
int main(int argc, char** argv)
{
    srand(42);  // fixed seed flicker patterns

    initRain();
    initFlicker();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WIN_W, WIN_H);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("Daffodil International University - Campus Scene");

    glClearColor(0.08f, 0.08f, 0.14f, 1.0f);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutSpecialUpFunc(specialUp);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}
