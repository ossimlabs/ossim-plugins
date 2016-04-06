/* Copyright (C) 2001-2015 Peter Selinger.
   This file is part of Potrace. It is free software and it is covered
   by the GNU General Public License. See the file COPYING for details. */

/* The GeoJSON backend of Potrace. */
/* Written 2012 by Christoph Hormann <chris_hormann@gmx.de> */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "potracelib.h"

/* return a point on a 1-dimensional Bezier segment */
static inline double bezier(double t, double x0, double x1, double x2, double x3) {
  double s = 1-t;
  return s*s*s*x0 + 3*(s*s*t)*x1 + 3*(t*t*s)*x2 + t*t*t*x3;
}

static char *format = "%.8f"; // for geographic point to millimeter precision

/* Convert a floating-point number to a string, using a pre-determined
   format. The format must be previously set with set_format().
   Returns one of a small number of statically allocated strings. */
static char *round_to_unit(double x) {
  static int n = 0;
  static char buf[4][100];

  n++;
  if (n >= 4) {
    n = 0;
  }
  sprintf(buf[n], format, x);
  return buf[n];
}

/* ---------------------------------------------------------------------- */
/* path-drawing auxiliary functions */

static potrace_dpoint_t cur;

static void geojson_moveto(FILE *fout, potrace_dpoint_t p)
{
  fprintf(fout, "[%s, %s]", round_to_unit(p.x), round_to_unit(p.y));
  cur = p;
}

static void geojson_lineto(FILE *fout, potrace_dpoint_t p)
{
  fprintf(fout, ", [%s, %s]", round_to_unit(p.x), round_to_unit(p.y));

  cur = p;
}

static void geojson_curveto(FILE *fout, potrace_dpoint_t p1, potrace_dpoint_t p2, potrace_dpoint_t p3)
{
  double step, t;
  int i;
  double x, y;

  step = 1.0 / 8.0;

  for (i=0, t=step; i<8; i++, t+=step) {
    x = bezier(t, cur.x, p1.x, p2.x, p3.x);
    y = bezier(t, cur.y, p1.y, p2.y, p3.y);

    fprintf(fout, ", [%s, %s]", round_to_unit(x), round_to_unit(y));
  }

  cur = p3;
}


static int geojson_path(FILE *fout, potrace_curve_t *curve) {
  int i=0;
  potrace_dpoint_t *c;
  
  int m = curve->n;
  if (m == 0)
    return 0;

  if ((m > 1) && (curve->tag[m-1] != POTRACE_ENDPOINT))
  {
     c = curve->c[m-1]; // Set last point in the curve as the first entry for closure
  }
  else
  {
     c = curve->c[0]; // This is a path that is not closed
     i=1;
  }
  geojson_moveto(fout, c[2]); // Write first point in the curve as the first entry for closure

  for (; i<m; i++) {
    c = curve->c[i];
    switch (curve->tag[i]) {
    case POTRACE_CORNER:
    case POTRACE_ENDPOINT:
      geojson_lineto(fout, c[1]);
      geojson_lineto(fout, c[2]);
      break;
    case POTRACE_CURVETO:
      geojson_curveto(fout, c[0], c[1], c[2]);
      break;
    }
  }

  return 0;
}


static void write_polygons(FILE *fout, potrace_path_t *tree, int first )
{
  potrace_path_t *p, *q;

  for (p=tree; p; p=p->sibling) {

     // Skip any entries with no points (possible after paths split due to edge encounter):
    if (p->curve.n != 0)
    {
       if (!first)
          fprintf(fout, ",\n");

       fprintf(fout, "{ \"type\": \"Feature\",\n");
       fprintf(fout, "  \"properties\": { },\n");
       fprintf(fout, "  \"geometry\": {\n");
       fprintf(fout, "    \"type\": \"Polygon\",\n");
       fprintf(fout, "    \"coordinates\": [\n");

       fprintf(fout, "      [");
       geojson_path(fout, &p->curve);
       fprintf(fout, " ]");

       // Traverse all siblings of this path's child:
       for (q=p->childlist; q; q=q->sibling)
       {
          fprintf(fout, ",\n      [");
          geojson_path(fout, &q->curve);
          fprintf(fout, " ]");
       }

       fprintf(fout, "    ]\n");
       fprintf(fout, "  }\n");
       fprintf(fout, "}");
    }
    for (q=p->childlist; q; q=q->sibling)
      write_polygons(fout, q->childlist, 0);

    first = 0;
  }
}

  static void write_line_strings(FILE *fout, potrace_path_t *tree, int first )
  {
    potrace_path_t *p, *q;

    for (p=tree; p; p=p->next)
    {
       // Skip any entries with no points (possible after paths split due to edge encounter):
      if (p->curve.n != 0)
      {

         if (!first)
            fprintf(fout, ",\n");

         fprintf(fout, "{ \"type\": \"Feature\",\n");
         fprintf(fout, "  \"properties\": { },\n");
         fprintf(fout, "  \"geometry\": {\n");
         fprintf(fout, "    \"type\": \"LineString\",\n");
         fprintf(fout, "    \"coordinates\": [\n");

         geojson_path(fout, &p->curve);

         fprintf(fout, "    ]\n"); // close coordinates list
         fprintf(fout, "  }\n"); // close geometry
         fprintf(fout, "}"); // close feature
      }
      first = 0;
    }
  }

/* ---------------------------------------------------------------------- */
/* Backend. */

/* public interface for GeoJSON */
int page_geojson(FILE *fout, potrace_path_t *plist, int as_polygons)
{
  /* header */
  fprintf(fout, "{\n");
  fprintf(fout, "\"type\": \"FeatureCollection\",\n");
  fprintf(fout, "\"features\": [\n");
  fflush(fout);

  if (as_polygons)
     write_polygons(fout, plist, 1);
  else
     write_line_strings(fout, plist, 1);

  /* write footer */
  fprintf(fout, "\n]\n");
  fprintf(fout, "}\n");

  fflush(fout);

  return 0;
}

