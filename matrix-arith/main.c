#include <stdio.h>

#include "matrix.h"

#define TEST_GAUSS
#define TEST_LU_DCMP
#define TEST_DET
// #define TEST_ERROR
#define TEST_LU_SLV
#define TEST_INV
#define TEST_LINMOD

void test_gauss(int r, int c, double *inp) {
    Matrix *a = matrix_alloc(r, c);
    Matrix *b = matrix_alloc(r, c);

    matrix_from_array(a, inp);
    matrix_gauss_elim(b, a);

    puts("before row reduction");
    matrix_view(a);
    puts("after row reduction");
    matrix_view(b);

    matrix_free(a);
    matrix_free(b);
}

void test_lup(int n, double *inp) {
    Matrix *a, *l, *u, *p, *b, *c, *d;

    a = matrix_alloc(n, n);
    l = matrix_alloc(n, n);
    u = matrix_alloc(n, n);
    p = matrix_alloc(n, n);
    b = matrix_alloc(n, n);
    c = matrix_alloc(n, n);
    d = matrix_alloc(n, n);

    matrix_from_array(a, inp);

    matrix_lu_dcmp(l, u, p, a);
    
    puts("test if factorization works");
    puts("P = ");
    matrix_view(p);
    puts("L = ");
    matrix_view(l);
    puts("U = ");
    matrix_view(u);

    puts("checking the result: A = trans(P)*L*U");

    matrix_transpose(b, p);
    matrix_mul(c, l, u);
    matrix_mul(d, b, c);
    matrix_view(d);
    puts("and A = ");
    matrix_view(a);

    matrix_free(a);
    matrix_free(l);
    matrix_free(u);
    matrix_free(p);
    matrix_free(b);
    matrix_free(c);
    matrix_free(d);
}

void test_det(int n, double *inp) {
    Matrix *a = matrix_alloc(n, n);
    matrix_from_array(a, inp);

    printf("Det: %.4f\n", matrix_det(a));

    matrix_free(a);
}

void test_error(void) {
    Matrix *a = matrix_alloc(4, 4);
    Matrix *b = NULL;
    matrix_copy(b, a);
    matrix_free(a);
    matrix_free(b);
}

void test_lusol(int n, double *inpa, double *inpb) {
    Matrix *a = matrix_alloc(n, n);
    Matrix *b = matrix_alloc(n, 1);
    matrix_from_array(a, inpa);
    matrix_from_array(b, inpb);
    /*matrix_view(a, "%10.5f ");
    matrix_view(b, "%10.5f ");*/
    Matrix *c = matrix_alloc(n, 1);
    matrix_solve(c, a, b);
    puts("sol =");
    matrix_view(c);
    matrix_free(a);
    matrix_free(b);
    matrix_free(c);
}

void test_inv(int n, double *inpa) {
    Matrix *a = matrix_alloc(n, n);
    matrix_from_array(a, inpa);

    Matrix *c = matrix_alloc(n, n);
    matrix_inv(c, a);
    puts("inv = ");
    matrix_view(c);
    Matrix *d = matrix_alloc(n, n);
    matrix_mul(d, a, c);
    puts("inv * main = ");
    matrix_view(d);
    matrix_free(a);
    /*matrix_free(b);*/
    matrix_free(c);
    matrix_free(d);
}

#ifdef TEST_LINMOD

/* least square method

a = mat(n x m), b = mat(n, 1)

*/
void least_squares(Matrix *res, Matrix *a, Matrix *b) {
    /* just test if it's working, not going to declare exceptions here */
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

    least_squares(c, a, b);
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

#endif

int main(void) {

#ifdef TEST_LINMOD
    // A = np.array([[1, 1], [1, 4], [1, 9], [1, 16]])
    // b = [2, 2, 5, 8]
    test_ls(4, 2,
        (double[]) {1, 1, 1, 4, 1, 9, 1, 16},
        (double[]) {2, 2, 5, 8});

    model(4,
        (double[]) {1, 2, 3, 4},
        (double[]) {2, 2, 5, 8});
#endif

#ifdef TEST_GAUSS

    test_gauss(3, 4, (double[]) {
        4, 5, 7,/**/6, 1, 4, /**/7, 0, 2, /**/5, 4, 9
    });

#endif

#ifdef TEST_LU_DCMP

    test_lup(3, (double[]) {
        1, 6, 2, /**/2, 12, 5, /**/-1, -3, -1
    });
    test_lup(4, (double[]) {
        2, 1, 3, 4,/**/-2, 1, -1, -2,/**/4, 4, 5, 11,/**/-2, 1, -7, -1
    });
    test_lup(3, (double[]) {
        2, 4, 2,/**/1, 1, 2,/**/-1, 0, 2
    });
    test_lup(3, (double[]) {
        2, 4, 2,/**/1, 5, 2,/**/4, -1, 9
    });
    test_lup(4, (double[]) {
        4, 3, 2, 1,/**/1, 10, 3, 4,/**/5, 3, 2, -4,/**/4, 8, 7, 9
    });

#endif

#ifdef TEST_DET
    test_det(2, (double[]) {
        9, 0,/**/2, -5
    });
    test_det(3, (double[]) {
        1, -3, -2,/**/3, 2, -1,/**/-1, 5, 0
    });
    test_det(4, (double[]) {
        -2, 2, 0, 1,/**/2, -1, 3, 0,/**/-1, 0, 2, -4,/**/0, -3, 5, 3
    });
    test_det(2, (double[]) {
        3, 4,/**/2, -5
    });
    test_det(2, (double[]) {
        17, -11,/**/6, -3
    });
    test_det(3, (double[]) {
        1, 1, 2,/**/2, 3, 1,/**/3, 4, -5
    });
    test_det(3, (double[]) {
        15, 4, 8,/**/-12, -7, 5,/**/0, -5, 15
    });
    test_det(4, (double[]) {
        1, 2, 3, 3,/**/2, 1, 1, 1,/**/3, 6, 5, 4,/**/3, 3, 2, 2
    });
    test_det(4, (double[]) {
        3, 3, 3, 1,/**/2, 4, 5, 2,/**/3, 4, 5, 1,/**/2, 2, 3, 4
    });
    test_det(2, (double[]) {
        0, -2,/**/6, 30
    });
    test_det(2, (double[]) {
        -27, 54,/**/24, -13
    });
    test_det(3, (double[]) {
        1, 2, 4,/**/2, 7, 3,/**/3, 1, -5
    });
    test_det(3, (double[]) {
        -2, 5, 8,/**/-7, 1, 12,/**/-6, 5, 17
    });
    test_det(4, (double[]) {
        0, 0, 3, 3,/**/3, 0, 1, 2,/**/1, 0, 2, 4,/**/2, 1, 3, 2
    });
    test_det(4, (double[]) {
        1, 1, 1, 1,/**/1, 2, 3, 4,/**/1, 3, 6, 10,/**/1, 4, 10, 20
    });
    test_det(4, (double[]) {
        4, 3, 2, 1,/**/1, 10, 3, 4,/**/5, 3, 2, -4,/**/4, 8, 7, 9
    });
#endif

#ifdef TEST_ERROR
    test_error();
#endif

#ifdef TEST_LU_SLV
    test_lusol(4, (double[]) {
        1, 1, 1, 1,/**/1, 2, 4, 8,/**/1, 3, 9, 27,/**/1, 4, 16, 64
    }, (double[]) {
        2, 2, 5, 8
    });
    test_lusol(4, (double[]) {
        1, -3, 1, 1,/**/3, -4, 1, -5,/**/0, 2, -1, 1,/**/2, 0, 0, 1
    }, (double[]) {
        2, 0, 1, 12
    });
    test_lusol(4, (double[]) {
        2, 2, -1, 1,/**/4, 3, -1, 2,/**/8, 5, -3, 4,/**/3, 3, -2, 2
    }, (double[]) {
        4, 6, 12, 6
    });
    test_lusol(4, (double[]) {
        2, 3, 11, 5,/**/1, 1, 5, 2,/**/2, 1, -3, 2,/**/1, 1, -3, 4
    }, (double[]) {
        2, 1, -3, -3
    });
    test_lusol(4, (double[]) {
        2, 5, 4, 1,/**/1, 3, 2, 1,/**/2, 10, 9, 7,/**/3, 8, 9, 2
    }, (double[]) {
        20, 11, 40, 37
    });
    /*test_lusol(4, (double[]) {
        1, -3, -26, 22, 1, 0, -8, 7, 1, 1, -2, 2, 4, 5, -2, 3
    }, (double[]) {
        0, 0, 0, 0
    });*/
#endif

#ifdef TEST_INV
    test_inv(4, (double[]) {
        2, 2, -1, 1,/**/4, 3, -1, 2,/**/8, 5, -3, 4,/**/3, 3, -2, 2
    });
    test_inv(3, (double[]) {
        972424.24, 924792.34, 12323243.824824,
        79739274.42, 83829213.92893, 810218038.9238,
        7191712.913313, 0.9274927492, 713973923.29
    });
    test_inv(3, (double[]) {
        0.000097242424, 0.00000092479234, 0.00000012323243824824,
        0.0000017973927442, 0.0008382921392893, 0.00008102180389238,
        0.00007191712913313, 0.00009274927492, 0.00071397392329
    });
#endif
    
    return 0;
}