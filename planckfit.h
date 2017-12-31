#ifndef PLANCKFIT_H
#define PLANCKFIT_H

#include "polyandplanckfit.h"

bool findParam(double x0[3],  const PointVal* points, int count, double out_x[3], double* out_fval);

#endif // PLANCKFIT_H
