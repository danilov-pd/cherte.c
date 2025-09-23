#include <stdio.h>
#define OLIVEC_IMPLEMENTATION
#include "olive.c"

#define CHERTEC_IMPLEMENTATION
#include "cherte.c"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

#define WIDTH 900
#define HEIGHT 600
// main program pixel buffer
uint32_t pixels[WIDTH*HEIGHT];

int main(void)
{
    Olivec_Canvas oc = olivec_canvas(pixels, WIDTH, HEIGHT, WIDTH);

    olivec_fill(oc, 0xFFFFFFFF);

    Chertec_Plot_Settings cps;

    chertec_default_settings(&cps);

    const int smpl = 101;
    const int smpl2 = 31;
    double xdata[smpl];
    double xdata2[smpl2];
    double ydata[smpl];
    double ydata2[smpl2];

    for (int i = 0; i < smpl; ++i) {
        xdata[i] = (double)i * 2 * M_PI / (smpl - 1);
        ydata[i] = sin(xdata[i]);
        
    }
    
    for (int i = 0; i < smpl2; ++i) {
        xdata2[i] = (double)i * 2 * M_PI / (smpl2 - 1);
        ydata2[i] = sin(xdata2[i] - M_PI / 6);
    }

    cps.oc = oc;
    CHERTEC_SET_RANGE(cps.xrange, 0.5, 2 * M_PI);
    CHERTEC_SET_RANGE(cps.yrange, -1.1, +1.1);
    cps.xticks = CHERTEC_DECIMAL(1, -1);
    cps.yticks = CHERTEC_DECIMAL(1, -1);
    cps.ticks_length = 10;
    cps.big_ticks_length = 16;
    cps.xbigticks = 5;
    cps.ybigticks = 1;
    CHERTEC_SET_FORMAT(cps.xformat, "%.1f");
    CHERTEC_SET_FORMAT(cps.yformat, "%.1f");

    cps.xdata = CHERTEC_POINTS(xdata, smpl);
    cps.ydata = CHERTEC_POINTS(ydata, smpl);

    puts("Plotting...");
    if (chertec_plot_data(cps)) {
        puts("Data plot for Series 1 successfull.");
    }

    cps.xdata = CHERTEC_POINTS(xdata2, smpl2);
    cps.ydata = CHERTEC_POINTS(ydata2, smpl2);
    cps.line_color = OLIVEC_RGBA(180,180,255,20);
    cps.line_thickness = 3;
    cps.point_color = OLIVEC_RGBA(0,145,145,255);
    cps.point_size = 4;
    
    if (chertec_plot_data(cps)) {
        puts("Data plot for Series 2 successfull.");
    }

    puts("Drawing axis...");
    if (chertec_draw_axis(cps)) {
        puts("Drawn axis successfully.");
    }

    olivec_text(cps.oc, "cherte.c", WIDTH / 2 - 192, HEIGHT / 2 - 24, olivec_default_font, 8, OLIVEC_RGBA(0,0,0,255));
    
    puts("Saving to png file...");
    const char *file_path = "plot.png";
    if (!stbi_write_png(file_path, WIDTH, HEIGHT, 4, pixels, sizeof(uint32_t)*WIDTH)) {
        fprintf(stderr, "ERROR: could not write %s\n", file_path);
        return 1;
    }
    puts("File saved.");
    
    return 0;
}
