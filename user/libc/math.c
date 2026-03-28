#include "include/math.h"

/* Implementaciones extremadamente basicas para demo */

double pow(double base, double exp) {
    double res = 1.0;
    for (int i = 0; i < (int)exp; i++) res *= base;
    return res;
}

double sqrt(double x) {
    if (x < 0) return 0;
    double res = x / 2.0;
    for (int i = 0; i < 10; i++) {
        res = 0.5 * (res + x / res);
    }
    return res;
}
