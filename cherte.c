#ifndef CHERTE_C_
#define CHERTE_C_

#ifndef OLIVE_C_
#error "olive.c header file must be provided for cherte.c"
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#ifndef CHERTECDEF
#define CHERTECDEF static inline
#endif

// Global CHERTEC defines.
// Max amount of tics per axis
#define CHERTEC_MAX_TICKS 1000
#define CHERTEC_FORMAT_STRING_SIZE 32
#define CHERTEC_STRING_BUFFER_SIZE 128

#define CHERTEC_CHANGE_ALPHA(color, a) (OLIVEC_RGBA(0, 0, 0, ((a) & 0xff)) | (OLIVEC_RGBA(0xff, 0xff, 0xff, 0) & (color)))
#define CHERTEC_SET_RANGE(range, from, to) do {                     \
                                               (range)[0] = (from); \
                                               (range)[1] = (to);   \
                                           } while(0);

#define CHERTEC_POINTS(dat, sz) ((Chertec_Points) {.data = (dat), .size = (sz)})
#define CHERTEC_DECIMAL(ma, ex) ((Chertec_Decimal) {.mant = (ma), .exp = (ex)})
#define CHERTEC_SET_FORMAT(dest, format) strncpy((dest), (format), (32)) 

typedef double Chertec_Real;

// This is a dynamic array struct for axis points.
// Users may populate and housekeep it however they like.
typedef struct {
    Chertec_Real *data;
    size_t size;
} Chertec_Points;


// Decimal floating point number
// represented by mant * 10^(exp)
typedef struct {
    int32_t mant;
    int8_t exp;
} Chertec_Decimal;

typedef struct {
    Olivec_Canvas oc;           // OLIVEC canvas
    Chertec_Points xdata;       // x coordinates data
    Chertec_Points ydata;       // y coordinates data
    Chertec_Real xrange[2];     // range for x axis to display
    Chertec_Real yrange[2];     // range for y axis to display
    Chertec_Decimal xticks;        // length between ticks on x axis
    int xbigticks;                 // big tick each xbigticks ticks
    Chertec_Decimal yticks;        // length between ticks on y axis
    int ybigticks;                 // big tick each ybigticks ticks
    int ticks_length;
    int big_ticks_length;
    char xformat[CHERTEC_FORMAT_STRING_SIZE]; // format for x tics
    char yformat[CHERTEC_FORMAT_STRING_SIZE]; // format for y tics
    int line_thickness;         // line thickness. if <= 0 then no lines
    uint32_t line_color;        // line color in OLIVEC format (ABGR in LE)
    int point_size;             // point size. if <= 0 then no points
    uint32_t point_color;       // point color in OLIVEC format (ABGR in LE)
} Chertec_Plot_Settings;

// Public functions' prototypes
CHERTECDEF bool chertec_plot_data(Chertec_Plot_Settings cps);
CHERTECDEF bool chertec_draw_axis(Chertec_Plot_Settings cps);

// Private functions' prototypes
CHERTECDEF bool __chertec_validate_ticks(Chertec_Plot_Settings cps);
CHERTECDEF void __chertec_blend_pixel_color(Olivec_Canvas oc, int x, int y, uint32_t color);
CHERTECDEF void __chertec_line(Olivec_Canvas oc, int x1, int y1, int x2, int y2, int thick, uint32_t color);
CHERTECDEF void __chertec_draw_xticks(Chertec_Plot_Settings cps, int x, Chertec_Real num, bool is_big_tick);
CHERTECDEF void __chertec_draw_yticks(Chertec_Plot_Settings cps, int y, Chertec_Real num, bool is_big_tick);
CHERTECDEF void __chertec_draw_ticks(Chertec_Plot_Settings cps, size_t max_size, Chertec_Real *range, Chertec_Decimal ticks, int big_ticks, void (*drawfunc)(Chertec_Plot_Settings, int, Chertec_Real, bool));

#endif // CHERTE_C_

#ifdef CHERTEC_IMPLEMENTATION

// Public funcions' implementations
#define MIN(a,b) (((a)<(b))?(a):(b))
CHERTECDEF bool chertec_plot_data(const Chertec_Plot_Settings cps)
{

    if (!__chertec_validate_ticks(cps)) { return false; }

    const size_t length = MIN(cps.xdata.size, cps.ydata.size);
    const size_t width = cps.oc.width;
    const size_t height = cps.oc.height;

    // Calculating scaling for the plot.
    const Chertec_Real xscale = width / (cps.xrange[1] - cps.xrange[0]);
    const Chertec_Real yscale = height / (cps.yrange[1] - cps.yrange[0]);
    
    for (size_t i = 0; i < length; ++i) {

        const int x = lround((cps.xdata.data[i] - cps.xrange[0]) * xscale);
        const int y = lround((cps.yrange[1] - cps.ydata.data[i]) * yscale);

        if ((i + 1 < length) && (cps.line_thickness > 0)) {
            // draw a line
            const int x1 = lround((cps.xdata.data[i + 1] - cps.xrange[0]) * xscale);
            const int y1 = lround((cps.yrange[1] - cps.ydata.data[i + 1]) * yscale);

            __chertec_line(cps.oc, x, y, x1, y1, cps.line_thickness, cps.line_color);
        }

        if (cps.point_size > 0) {
            // draw a point
            olivec_circle(cps.oc, x, y, cps.point_size, cps.point_color);
        }
    }
    return true;
}
#undef MIN

CHERTECDEF bool chertec_draw_axis(Chertec_Plot_Settings cps)
{

    if (!__chertec_validate_ticks(cps)) { return false; }

    // draw axis
    __chertec_line(cps.oc, 0, -2, 0, cps.oc.height + 2, 1, OLIVEC_RGBA(0,0,0,255));
    __chertec_line(cps.oc, -2, cps.oc.height - 1, cps.oc.width + 2, cps.oc.height - 1, 1, OLIVEC_RGBA(0,0,0,255));

    __chertec_draw_ticks(cps, cps.oc.width, cps.xrange, cps.xticks, cps.xbigticks, __chertec_draw_xticks);
    __chertec_draw_ticks(cps, cps.oc.height, cps.yrange, cps.yticks, cps.ybigticks, __chertec_draw_yticks);

/*
    const size_t width = cps.oc.width;
    const size_t height = cps.oc.height;
    const Chertec_Real xdelta = cps.xrange[1] - cps.xrange[0];
    const Chertec_Real ydelta = cps.yrange[1] - cps.yrange[0];
    const Chertec_Real xscale = width / xdelta;

    const int32_t left_exp = (int32_t)floor(log10(cps.xrange[0]));
    const Chertec_Real left_mantissa = cps.xrange[0] * pow(10, left_exp);
    const Chertec_Real exp_diff = pow(10, left_exp - cps.xticks.exp);
    const Chertec_Real left_rounded = round(cps.xrange[0] * exp_diff);

    const int32_t xdelta_exp = (int32_t)floor(log10(xdelta));
    const Chertec_Real xdelta_mantissa = xdelta * pow(10, xdelta_exp);
    const Chertec_Real exp_diff_1 = pow(10, xdelta_exp - cps.xticks.exp);
    const int32_t xdelta_int = lround(xdelta * exp_diff_1);

    for (int32_t dx = cps.xticks.mant; dx < xdelta_int; dx += cps.xticks.mant) {
        Chertec_Real x = left_rounded + dx * pow(10, cps.xticks.exp);
        x = x * xscale - cps.xrange[0];
        __chertec_line(cps.oc, x, cps.oc.height - 1 - cps.ticks_length, x, cps.oc.height - 1, 1, OLIVEC_RGBA(0,0,0,255));
    }

*/

    // Calculating scaling for the plot.
//    const Chertec_Real xscale = width / (cps.xrange[1] - cps.xrange[0]);
//    const Chertec_Real yscale = height / (cps.yrange[1] - cps.yrange[0]);

//    const int x = lround((cps.xdata.data[i] - cps.xrange[0]) * xscale);

    // draw ticks
    // x axis
    

    // y axis
    
    return true;
}

CHERTECDEF void chertec_default_settings(Chertec_Plot_Settings *cps)
{
    cps->xticks = CHERTEC_DECIMAL(1, 0);
    cps->yticks = CHERTEC_DECIMAL(1, 0);
    cps->line_thickness = 1;
    cps->line_color = OLIVEC_RGBA(0,0,0,255);
    cps->point_size = 2;
    cps->point_color = OLIVEC_RGBA(255,0,0,255);
    cps->ticks_length = 5;

    cps->oc = (Olivec_Canvas){.pixels = NULL, .width = 0, .height = 0, .stride = 0};
    cps->xdata = (Chertec_Points){.data = NULL, .size = 0};
    cps->ydata = (Chertec_Points){.data = NULL, .size = 0};

    CHERTEC_SET_FORMAT(cps->xformat, "%.2f");
    CHERTEC_SET_FORMAT(cps->yformat, "%.2f");
}


// Private functions' implementations
CHERTECDEF bool __chertec_validate_ticks(Chertec_Plot_Settings cps)
{
    const Chertec_Real xticks = cps.xticks.mant * pow(10, cps.xticks.exp);
    const Chertec_Real yticks = cps.yticks.mant * pow(10, cps.yticks.exp);

    const Chertec_Real xdelta = cps.xrange[1] - cps.xrange[0];
    const Chertec_Real ydelta = cps.yrange[1] - cps.yrange[0];

    // Checking xrange and yrange ordering
    if (xdelta <= 0 || ydelta <= 0) {
        return false;
    }
    
    // Checking xticks and yticks and potential zero division
    if (xticks <= 0 || yticks <= 0) {
        return false;
    }
     
    // Checking whether we get <1 amount of ticks
    if (xticks > xdelta || yticks > ydelta) {
        return false;
    }
    
    // Checking whether the amount of ticks exceeds CHERTEC_MAX_TICKS
    if (xdelta / xticks > CHERTEC_MAX_TICKS ||
        ydelta / yticks > CHERTEC_MAX_TICKS) {
        return false;
    }

    return true;
}

CHERTECDEF void __chertec_blend_pixel_color(Olivec_Canvas oc, int x, int y, uint32_t color)
{
    if (x >= 0 && x < oc.width &&
        y >= 0 && y < oc.height) {
        olivec_blend_color(&OLIVEC_PIXEL(oc, x, y), color);
    }
}

CHERTECDEF void __chertec_draw_xticks(Chertec_Plot_Settings cps, int x, Chertec_Real num, bool is_big_tick) {
    char buf[CHERTEC_STRING_BUFFER_SIZE];
    int tick_length = cps.ticks_length;
    if (is_big_tick) { tick_length = cps.big_ticks_length; }

    __chertec_line(cps.oc, x, cps.oc.height - 1 - tick_length, x, cps.oc.height - 1, 1, OLIVEC_RGBA(0,0,0,255));

    if (is_big_tick) {
        snprintf(buf, CHERTEC_STRING_BUFFER_SIZE, cps.xformat, num);
        olivec_text(cps.oc, buf, x, cps.oc.height - 1 - tick_length - 8, olivec_default_font, 1, OLIVEC_RGBA(0,0,0,255));
    }

}

CHERTECDEF void __chertec_draw_yticks(Chertec_Plot_Settings cps, int y, Chertec_Real num, bool is_big_tick) {
    char buf[CHERTEC_STRING_BUFFER_SIZE];
    int tick_length = cps.ticks_length;
    if (is_big_tick) { tick_length = cps.big_ticks_length; }

    __chertec_line(cps.oc, 0, cps.oc.height - y, tick_length, cps.oc.height - y, 1, OLIVEC_RGBA(0,0,0,255));

    if (is_big_tick) {
        snprintf(buf, CHERTEC_STRING_BUFFER_SIZE, cps.xformat, num);
        olivec_text(cps.oc, buf, tick_length + 4, cps.oc.height - y - 3, olivec_default_font, 1, OLIVEC_RGBA(0,0,0,255));
    }
}

CHERTECDEF void __chertec_draw_ticks(Chertec_Plot_Settings cps, size_t max_size, Chertec_Real *range, Chertec_Decimal ticks, int big_ticks, void (*drawfunc)(Chertec_Plot_Settings, int, Chertec_Real, bool))
{
    const Chertec_Real delta = range[1] - range[0];
    const Chertec_Real scale = max_size / delta;

    // TODO: all of this log10 stuff is suboptimal and should be replaced by
    // a fair approximation. see stbsp__real_to_str func in stb_sprintf.h
    // for reference. ideally we should stride for no linkage to math library
    const int32_t left_exp = (int32_t)floor(log10(OLIVEC_ABS(Chertec_Real, range[0])));
    const Chertec_Real left_mantissa = range[0] * pow(10, -left_exp);
    const Chertec_Real exp_diff = pow(10, left_exp - ticks.exp);
    const Chertec_Real left_rounded = round(left_mantissa * exp_diff) * pow(10, ticks.exp);

    const int32_t delta_exp = (int32_t)floor(log10(delta));
    const Chertec_Real delta_mantissa = delta * pow(10, delta_exp);
    const Chertec_Real exp_diff_1 = pow(10, delta_exp - ticks.exp);
    const int32_t delta_int = lround(delta * exp_diff_1);

    for (int32_t dx = ticks.mant, i = 1; dx < delta_int; dx += ticks.mant, ++i) {
        Chertec_Real x = left_rounded + dx * pow(10, ticks.exp);    
        Chertec_Real x_scaled = dx * pow(10, ticks.exp) * scale;
        drawfunc(cps, x_scaled, x, (bool) (i % big_ticks == 0));
    }
}

// __chertec_line is derived from plotLineWidth procedure from bresenham.c
//
// bresenham.c is created by Alois Zingl
// Distributed under MIT license.
// Get the latest version from here:
// https://github.com/zingl/Bresenham
// 
// MIT License
// 
// Copyright (c) 2020 zingl
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// TODO: maybe replace by the '8.1.1 Improved thick line algorithm' from page 84
// of https://zingl.github.io/Bresenham.pdf to get rid of sqrt func as well.
#define MAX(a,b) (((a)>(b))?(a):(b))
CHERTECDEF void __chertec_line(Olivec_Canvas oc, int x0, int y0, int x1, int y1, int wd, uint32_t color)
{                                    /* plot an anti-aliased line of width wd */
   wd *= 2; // TODO: research why width doubling is needed;
   int dx = OLIVEC_ABS(int, x1-x0), sx = x0 < x1 ? 1 : -1;
   int dy = OLIVEC_ABS(int, y1-y0), sy = y0 < y1 ? 1 : -1;
   int err = dx-dy, e2, x2, y2;                           /* error value e_xy */
   // TODO: maybe replace this sqrt with sufficiently good approximation to
   // get rid of math.h inclusion. There is no math.h in olive.c.
   float ed = dx+dy == 0 ? 1 : sqrt((float)dx*dx+(float)dy*dy);

   for (wd = (wd+1)/2; ; ) {                                    /* pixel loop */
      __chertec_blend_pixel_color(oc, x0, y0, CHERTEC_CHANGE_ALPHA(color, 255 - (MAX(0,(int)roundf(255*(OLIVEC_ABS(int, err-dx+dy)/ed-wd+1))))));
      e2 = err; x2 = x0;
      if (2*e2 >= -dx) {                                            /* x step */
         for (e2 += dy, y2 = y0; e2 < ed*wd && (y1 != y2 || dx > dy); e2 += dx)
            __chertec_blend_pixel_color(oc, x0, y2 += sy, CHERTEC_CHANGE_ALPHA(color, 255 - (MAX(0,(int)roundf(255*(OLIVEC_ABS(int, e2)/ed-wd+1))))));
         if (x0 == x1) break;
         e2 = err; err -= dy; x0 += sx;
      }
      if (2*e2 <= dy) {                                             /* y step */
         for (e2 = dx-e2; e2 < ed*wd && (x1 != x2 || dx < dy); e2 += dy)
            __chertec_blend_pixel_color(oc, x2 += sx, y0, CHERTEC_CHANGE_ALPHA(color, 255 - (MAX(0,(int)roundf(255*(OLIVEC_ABS(int, e2)/ed-wd+1))))));
         if (y0 == y1) break;
         err += dx; y0 += sy;
      }
   }
}
#undef MAX

#endif // CHERTEC_IMPLEMENTATION


