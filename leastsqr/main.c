#include <stdio.h>

#include "matrix.h"

void llsq(Matrix *res, Matrix *a, Matrix *b) {
    Matrix *at = matrix_alloc(a->col, a->row);
    matrix_transpose(at, a);
    
    Matrix *at_a = matrix_alloc(a->col, a->col);
    matrix_mul(at_a, at, a);
    Matrix *at_b = matrix_alloc(a->col, 1);
    matrix_mul(at_b, at, b);

    Matrix *at_a_inv = matrix_alloc(a->col, a->col);
    matrix_inv(at_a_inv, at_a);

    matrix_mul(res, at_a_inv, at_b);

    matrix_free(at);
    matrix_free(at_a);
    matrix_free(at_b);
    matrix_free(at_a_inv);
}

void test_ls(int n, int m, double *araw, double *braw) {
    Matrix *a = matrix_alloc(n, m);
    Matrix *b = matrix_alloc(n, 1);
    matrix_from_array(a, araw);
    matrix_from_array(b, braw);
    Matrix *c = matrix_alloc(m, 1);

    llsq(c, a, b);
    matrix_view(c);
    matrix_free(a);
    matrix_free(b);
    matrix_free(c);
}

/* ~Y = theta1 + theta2 * X model */
void model(int n, double *x, double *y) {
    Matrix *a = matrix_alloc(n, 2);
    Matrix *b = matrix_alloc(n, 1);
    int i;

    for (i = 0; i < n; i++) {
        matrix_set(a, i, 0, 1);
        matrix_set(a, i, 1, x[i]);
    }
    matrix_from_array(b, y);

    Matrix *c = matrix_alloc(2, 1);
    printf("a =\n");
    matrix_view(a);
    printf("b =\n");
    matrix_view(b);
    printf("theta =\n");
    least_squares(c, a, b);
    matrix_view(c);

    double theta1 = matrix_get(c, 0, 0), theta2 = matrix_get(c, 1, 0);
    
    double norm = 0, diff;
    printf("r =\n");
    for (i = 0; i < n; i++) {
        diff = theta1 + theta2 * x[i] - y[i];
        printf("%g\n", diff);
        norm += diff * diff;
    }

    printf("norm = %g\n", norm);

    matrix_free(a);
    matrix_free(b);
    matrix_free(c);
}

int main(void) {
    test_ls(4, 2,
        (double[]) {1, 1, 1, 4, 1, 9, 1, 16},
        (double[]) {2, 2, 5, 8});

    model(4,
        (double[]) {1, 2, 3, 4},
        (double[]) {2, 2, 5, 8});

    return 0;
}