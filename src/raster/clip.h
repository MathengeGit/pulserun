#pragma once
#include "core/framebuffer.h"
#include "raster/polygon.h"

struct ClipRect {
    double xmin, ymin, xmax, ymax;
};

// Cohen-Sutherland line clipping. Modifies the end points in place and returns
// false if the line lies completely outside the window.
bool clipLineCohenSutherland(double& x0, double& y0, double& x1, double& y1, const ClipRect& r);

// Sutherland-Hodgman polygon clipping against a rectangular window.
Polygon clipPolygonSutherlandHodgman(const Polygon& in, const ClipRect& r);

// Convenience wrappers that clip and then draw.
void drawLineClipped(Framebuffer& fb, double x0, double y0, double x1, double y1, const ClipRect& r, Color c);
void fillPolygonClipped(Framebuffer& fb, const Polygon& p, const ClipRect& r, Color c);
