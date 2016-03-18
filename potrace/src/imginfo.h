#ifndef IMGINFO_H
#define IMGINFO_H

#include "trans.h"

/* structure to hold a dimensioned value */
struct dim_s {
  double x; /* value */
  double d; /* dimension (in pt), or 0 if not given */
};
typedef struct dim_s dim_t;

/* structure to hold per-image information, set e.g. by calc_dimensions */
struct imginfo_s {
  int pixwidth;        /* width of input pixmap */
  int pixheight;       /* height of input pixmap */
  double width;        /* desired width of image (in pt or pixels) */
  double height;       /* desired height of image (in pt or pixels) */
  double lmar, rmar, tmar, bmar;   /* requested margins (in pt) */
  trans_t trans;        /* specify relative position of a tilted rectangle */
};
typedef struct imginfo_s imginfo_t;


#endif /* IMGINFO_H */
