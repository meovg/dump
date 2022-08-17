#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "matrix.h"

/* error messages */

const char *matrix_errname(int err) {
    switch (err) {
    case E_MATRIX_SUCCESS:
        return "Success";
    case E_MATRIX_NGPTR:
        return "Invalid pointer";
    case E_MATRIX_NGSIZ:
        return "Invalid size";
    case E_MATRIX_NEQSIZ:
        return "Not in equal size";
    case E_MATRIX_NALLOC:
        return "Allocation failed";
    case E_MATRIX_OUTRANGE:
        return "Out of range";
    case E_MATRIX_NSQR:
        return "Not square matrix";
    case E_MATRIX_FACFAIL:
        return "Factorization failed";
    case E_MATRIX_SLVFAIL:
        return "Cannot solve";
    case E_MATRIX_DIVZERO:
        return "Division by zero";
    default:
        return "*shrugs*";
    }
}

/* matrix manipulation & operation */

Matrix *matrix_alloc(size_t row, size_t col) {
    if (row <= 0 || col <= 0) {
        MATRIX_ERROR("The sizes must be a positive integer", E_MATRIX_NGSIZ);
    }

    Matrix *mat = malloc(sizeof *mat);
    if (mat == NULL) {
        MATRIX_ERROR("Matrix allocation failed", E_MATRIX_NALLOC);
    }

    mat->row = row,
    mat->col = col;
    mat->data = malloc(row * col * sizeof *(mat->data));
    if (mat->data == NULL) {
        MATRIX_ERROR("Cannot allocate member array", E_MATRIX_NALLOC);
    }
    memset(mat->data, 0, row * col * sizeof *(mat->data));
    return mat;
}

void matrix_free(Matrix *mat) {
    free(mat->data);
    free(mat);
}

void matrix_from_array(Matrix *mat, const double *arr) {
    for (int i = 0; i < mat->row * mat->col; i++) {
        mat->data[i] = *(arr++);
    }
}

void matrix_copy(Matrix *dst, const Matrix *src) {
    if (dst->row != src->row || dst->col != src->col) {
        MATRIX_ERROR("Both matrices must have the same size", E_MATRIX_NEQSIZ);
    }
    for (int i = 0; i < dst->row * dst->col; i++) {
        dst->data[i] = src->data[i];
    }
}

void matrix_set(Matrix *mat, size_t i, size_t j, double val) {
    if (i < 0 || i >= mat->row) {
        MATRIX_ERROR("Index i is out of matrix row range", E_MATRIX_OUTRANGE);
    } else if (j < 0 || j >= mat->col) {
        MATRIX_ERROR("Index j is out of matrix column range", E_MATRIX_OUTRANGE);
    }
    mat->data[i * mat->col + j] = val;
}

void matrix_set_all(Matrix *mat, double val) {
    for (int i = 0; i < mat->row * mat->col; i++) {
        mat->data[i] = val;
    }
}

void matrix_set_identity(Matrix *mat) {
    int N = mat->row;
    if (mat->col != N) {
        MATRIX_ERROR("Matrix must be square", E_MATRIX_NSQR);
    }

    for (int i = 0; i < N; i++) {
        MATRIX_AT(mat, i, i) = 1.0;
    }
}

double *matrix_get_ptr(const Matrix *mat, size_t i, size_t j) {
    if (i < 0 || i >= mat->row) {
        MATRIX_ERROR("Index i is out of matrix row range", E_MATRIX_OUTRANGE);
    } else if (j < 0 || j >= mat->col) {
        MATRIX_ERROR("Index j is out of matrix column range", E_MATRIX_OUTRANGE);
    }
    return mat->data + i * mat->col + j;
}

const double *matrix_get_const_ptr(const Matrix *mat, size_t i, size_t j) {
    return (const double *)matrix_get_ptr(mat, i, j);
}

double matrix_get(const Matrix *mat, size_t i, size_t j) {
    return *matrix_get_ptr(mat, i, j);
}

void matrix_view(const Matrix *mat) {
    printf("%d x %d matrix:\n", mat->row, mat->col);
    for (int i = 0; i < mat->row; i++) {
        for (int j = 0; j < mat->col; j++) {
            printf("%13.5g", MATRIX_AT(mat, i, j));
        }
        puts("");
    }
}

void matrix_add(Matrix *dst, const Matrix *rhs) {
    int N = dst->row;
    int M = dst->col;

    if (rhs->row != N || rhs->col != M) {
        MATRIX_ERROR("Both matrices must have the same size", E_MATRIX_NEQSIZ);
    }
    for (int i = 0; i < N * M; i++) {
        dst->data[i] += rhs->data[i];
    }
}

void matrix_sub(Matrix *dst, const Matrix *rhs) {
    int N = dst->row;
    int M = dst->col;

    if (rhs->row != N || rhs->col != M) {
        MATRIX_ERROR("Both matrices must have the same size", E_MATRIX_NEQSIZ);
    }
    for (int i = 0; i < N * M; i++) {
        dst->data[i] -= rhs->data[i];
    }
}

void matrix_valmul(Matrix *dst, const Matrix *rhs) {
    int N = dst->row;
    int M = dst->col;

    if (rhs->row != N || rhs->col != M) {
        MATRIX_ERROR("Both matrices must have the same size", E_MATRIX_NEQSIZ);
    }
    for (int i = 0; i < N * M; i++) {
        dst->data[i] *= rhs->data[i];
    }
}

void matrix_valdiv(Matrix *dst, const Matrix *rhs) {
    int N = dst->row;
    int M = dst->col;

    if (rhs->row != N || rhs->col != M) {
        MATRIX_ERROR("Both matrices must have the same size", E_MATRIX_NEQSIZ);
    }
    for (int i = 0; i < N * M; i++) {
        dst->data[i] /= rhs->data[i];
    }
}

void matrix_mul(Matrix *dst, const Matrix *lhs, const Matrix *rhs) {
    int N = lhs->row;
    int P = lhs->col;
    int M = rhs->col;

    if (rhs->row != P) {
        MATRIX_ERROR("The number of columns in the left hand matrix must be "
            "equal to the number of rows in the right one",
            E_MATRIX_NEQSIZ);
    } else if (dst->row != N || dst->col != M) {
        MATRIX_ERROR("Invalid size of the product matrix", E_MATRIX_NEQSIZ);
    }

    for (int i = 0; i < N; i++) {
        for (int k = 0; k < P; k++) {
            for (int j = 0; j < M; j++) {
                MATRIX_AT(dst, i, j) += MATRIX_AT(lhs, i, k) * MATRIX_AT(rhs, k, j);
            }
        }
    }
}

void matrix_transpose(Matrix *dst, const Matrix *src) {
    int N = src->col;
    int M = src->row;

    if (dst->row != N || dst->col != M) {
        MATRIX_ERROR("Invalid size of transposed matrix", E_MATRIX_NEQSIZ);
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            MATRIX_AT(dst, i, j) = MATRIX_AT(src, j, i);
        }
    }
}

void value_swap(double *a, double *b) {
    double tmp = *a;
    *a = *b;
    *b = tmp;
}

void matrix_row_swap(Matrix *mat, size_t u, size_t v) {
    if (u < 0 || u >= mat->row) {
        MATRIX_ERROR("Index u is out of matrix row bound", E_MATRIX_OUTRANGE);
    } else if (v < 0 || v >= mat->row) {
        MATRIX_ERROR("Index v is out of matrix row bound", E_MATRIX_OUTRANGE);
    }

    for (int i = 0; i < mat->col; i++) {
        value_swap(&MATRIX_AT(mat, u, i), &MATRIX_AT(mat, v, i));
    }
}

void matrix_col_swap(Matrix *mat, size_t u, size_t v) {
    if (u < 0 || u >= mat->col) {
        MATRIX_ERROR("Index u is out of matrix column bound", E_MATRIX_OUTRANGE);
    } else if (v < 0 || v >= mat->col) {
        MATRIX_ERROR("Index v is out of matrix column bound", E_MATRIX_OUTRANGE);
    }

    for (int i = 0; i < mat->row; i++) {
        value_swap(&MATRIX_AT(mat, i, u), &MATRIX_AT(mat, i, v));
    }
}

void matrix_row_scale(Matrix *mat, size_t u, double scalar) {
    if (u < 0 || u >= mat->row) {
        MATRIX_ERROR("Index u is out of matrix row bound", E_MATRIX_OUTRANGE);
    }

    for (int i = 0; i < mat->col; i++) {
        MATRIX_AT(mat, u, i) *= scalar;
    }
}

void matrix_col_scale(Matrix *mat, size_t u, double scalar) {
    if (u < 0 || u >= mat->col) {
        MATRIX_ERROR("Index u is out of matrix column bound", E_MATRIX_OUTRANGE);
    }

    for (int i = 0; i < mat->row; i++) {
        MATRIX_AT(mat, i, u) *= scalar;
    }
}

void matrix_add_row_multiple(Matrix *mat, size_t u, size_t v, double scalar) {
    if (u < 0 || u >= mat->row) {
        MATRIX_ERROR("Index u is out of matrix row bound", E_MATRIX_OUTRANGE);
    } else if (v < 0 || v >= mat->row) {
        MATRIX_ERROR("Index v is out of matrix row bound", E_MATRIX_OUTRANGE);
    }

    for (int i = 0; i < mat->col; i++) {
        MATRIX_AT(mat, u, i) += scalar * MATRIX_AT(mat, v, i);
    }
}

void matrix_add_col_multiple(Matrix *mat, size_t u, size_t v, double scalar) {
    if (u < 0 || u >= mat->col) {
        MATRIX_ERROR("Index u is out of matrix column bound", E_MATRIX_OUTRANGE);
    } else if (v < 0 || v >= mat->col) {
        MATRIX_ERROR("Index v is out of matrix column bound", E_MATRIX_OUTRANGE);
    }

    for (int i = 0; i < mat->row; i++) {
        MATRIX_AT(mat, i, u) += scalar * MATRIX_AT(mat, i, v);
    }
}

int matrix_find_nonzero_row(const Matrix *mat, size_t cur_col, size_t st_row) {
    if (cur_col < 0 || cur_col >= mat->col) {
        MATRIX_ERROR("Current column index is out of bound", E_MATRIX_OUTRANGE);
    } else if (st_row < 0 || st_row >= mat->row) {
        MATRIX_ERROR("Starting row index is out of bound", E_MATRIX_OUTRANGE);
    }

    for (int i = st_row; i < mat->row; i++) {
        if (MATRIX_AT(mat, i, cur_col) != 0) {
            return i;
        }
    }
    return -1;
}

/* some few linear algebra functions */

void matrix_gauss_elim(Matrix *dst, const Matrix *src) {
    matrix_copy(dst, src);

    int cur_row = 0;
    for (int i = 0; i < dst->col; i++) {
        int k = matrix_find_nonzero_row(dst, i, cur_row);

        if (k != -1) {
            matrix_row_swap(dst, k, cur_row);
            matrix_row_scale(dst, cur_row, 1.0 / MATRIX_AT(dst, cur_row, i));
            for (int j = cur_row + 1; j < dst->row; j++) {
                matrix_add_row_multiple(dst, j, cur_row, -MATRIX_AT(dst, j, i));
            }
            cur_row++;
            if (cur_row >= dst->row) {
                break;
            }
        }
    }
}

void matrix_gaussjordan_elim(Matrix *dst, const Matrix *src) {
    matrix_copy(dst, src);

    int cur_row = 0;
    for (int i = 0; i < dst->col; i++) {
        int k = matrix_find_nonzero_row(dst, i, cur_row);

        if (k != -1) {
            matrix_row_swap(dst, k, cur_row);
            matrix_row_scale(dst, cur_row, 1.0 / MATRIX_AT(dst, cur_row, i));
            for (int j = 0; j < dst->row; j++) {
                if (j != cur_row) {
                    matrix_add_row_multiple(dst, j, cur_row, -MATRIX_AT(dst, j, i));
                }
            }
            cur_row++;
            if (cur_row >= dst->row) {
                break;
            }
        }
    }
}

double matrix_det(const Matrix *mat) {
    int N = mat->row;

    if (mat->col != N) {
        MATRIX_ERROR("Matrix must be square", E_MATRIX_NSQR);
    }

    Matrix *l = matrix_alloc(N, N);
    Matrix *u = matrix_alloc(N, N);
    Matrix *p = matrix_alloc(N, N);

    matrix_lu_dcmp(l, u, p, mat);
    double det = matrix_lu_det(l, u, p);

    matrix_free(l);
    matrix_free(u);
    matrix_free(p);

    return det;
}

void matrix_back_subst(Matrix *dst, const Matrix *a, const Matrix *b) {
    int N = a->row;
    int M = b->col;

    if (a->col != N) {
        MATRIX_ERROR("Left matrix must be square", E_MATRIX_NSQR);
    } else if (b->row != N) {
        MATRIX_ERROR("Invalid size of right vector", E_MATRIX_NEQSIZ);
    } else if (dst->row != N || dst->col != M) {
        MATRIX_ERROR("Invalid size of solution vector", E_MATRIX_NEQSIZ);
    }

    for (int k = 0; k < M; k++) {
        for (int i = N - 1; i >= 0; i--) {
            double tmp = MATRIX_AT(b, i, k);
            for (int j = i + 1; j < N; j++) {
                tmp -= MATRIX_AT(a, i, j) * MATRIX_AT(dst, j, k);
            }

            if (MATRIX_AT(a, i, i) == 0) {
                MATRIX_ERROR("Diagonal elements of left matrix must not be zero",
                    E_MATRIX_DIVZERO);
            }

            MATRIX_AT(dst, i, k) = tmp / MATRIX_AT(a, i, i);
        }
    }
}

void matrix_forward_subst(Matrix *dst, const Matrix *a, const Matrix *b) {
    int N = a->row;
    int M = b->col;

    if (a->col != N) {
        MATRIX_ERROR("Left matrix must be square", E_MATRIX_NSQR);
    } else if (b->row != N) {
        MATRIX_ERROR("Invalid size of right vector", E_MATRIX_NEQSIZ);
    } else if (dst->row != N || dst->col != M) {
        MATRIX_ERROR("Invalid size of solution vector", E_MATRIX_NEQSIZ);
    }

    for (int k = 0; k < M; k++) {
        for (int i = 0; i < N; i++) {
            double tmp = MATRIX_AT(b, i, k);
            for (int j = 0; j < i; j++) {
                tmp -= MATRIX_AT(a, i, j) * MATRIX_AT(dst, j, k);
            }

            if (MATRIX_AT(a, i, i) == 0) {
                MATRIX_ERROR("Diagonal elements of left matrix must not be zero",
                    E_MATRIX_DIVZERO);
            }

            MATRIX_AT(dst, i, k) = tmp / MATRIX_AT(a, i, i);
        }
    }
}

void matrix_solve(Matrix *dst, const Matrix *a, const Matrix *b) {
    int N = b->row;
    int M = b->col;

    if (dst->row != N || dst->col != M) {
        MATRIX_ERROR("Invalid size for solution vector", E_MATRIX_NGPTR);
    } else if (a->col != N || a->row != N) {
        MATRIX_ERROR("Left hand matrix must be square and its rank must be equal to "
            "the number of variables (no. of rows on right hand matrix",
            E_MATRIX_NEQSIZ);
    }

    Matrix *l = matrix_alloc(N, N);
    Matrix *u = matrix_alloc(N, N);
    Matrix *p = matrix_alloc(N, N);

    matrix_lu_dcmp(l, u, p, a);
    matrix_lu_solve(dst, l, u, p, b);

    matrix_free(l);
    matrix_free(u);
    matrix_free(p);
}

void matrix_inv(Matrix *dst, const Matrix *src) {
    int N = src->row;

    if (src->col != N) {
        MATRIX_ERROR("Source matrix must be square", E_MATRIX_NEQSIZ);
    } else if (dst->col != N || dst->row != N) {
        MATRIX_ERROR("Destination and source matrix must have equal size",
            E_MATRIX_NEQSIZ);
    }

    Matrix *l = matrix_alloc(N, N);
    Matrix *u = matrix_alloc(N, N);
    Matrix *p = matrix_alloc(N, N);

    matrix_lu_dcmp(l, u, p, src);
    matrix_lu_inv(dst, l, u, p);

    matrix_free(l);
    matrix_free(u);
    matrix_free(p);
}

/* LU decompostion and related functions */

void matrix_lu_dcmp(Matrix *l, Matrix *u, Matrix *p, const Matrix *src) {
    int N = src->row;

    if (src->col != N) {
        MATRIX_ERROR("Source matrix must be square", E_MATRIX_NSQR);
    } else if (l->row != N || l->col != N 
        || u->row != N || u->col != N
        || p->row != N || p->col != N)
    {
        MATRIX_ERROR("Decomposition matrices must have the same size as the source",
            E_MATRIX_NEQSIZ);
    }

    matrix_set_identity(p);
    matrix_set_identity(l);
    matrix_copy(u, src);

    for (int i = 0; i < N - 1; i++) {
        int k = matrix_find_nonzero_row(u, i, i);
        if (k == -1) {
            MATRIX_ERROR("Matrix cannot be LU-decomposed", E_MATRIX_FACFAIL);
        }

        matrix_row_swap(p, k, i);
        matrix_row_swap(u, k, i);

        for (int j = 0; j <= i - 1; j++) {
            value_swap(&MATRIX_AT(l, i, j), &MATRIX_AT(l, k, j));
        }

        for (int j = i + 1; j < N; j++) {
            MATRIX_AT(l, j, i) = MATRIX_AT(u, j, i) / MATRIX_AT(u, i, i);
            matrix_add_row_multiple(u, j, i, -MATRIX_AT(l, j, i));
        }
    }
}

double matrix_lu_det(const Matrix *l, const Matrix *u, const Matrix *p) {
    int N = l->row;

    if (l->col != N
        || u->row != N || u->col != N 
        || p->row != N || p->col != N)
    {
        MATRIX_ERROR("All matrices passed must be square and have the same size",
            E_MATRIX_NEQSIZ);
    }

    int perm_cnt = 0;
    double det = 1.0;

    for (int i = 0; i < N; i++) {
        det *= MATRIX_AT(u, i, i);
        perm_cnt += (MATRIX_AT(p, i, i) != 1.0);
    }
    if (perm_cnt > 0 && (perm_cnt & 1) == 0) {
        det *= -1;
    }
    return det;
}

void matrix_lu_solve(Matrix *dst, const Matrix *l, const Matrix *u,
                     const Matrix *p, const Matrix *b)
{
    int N = b->row;
    int M = b->col;

    if (dst->row != N || dst->col != M) {
        MATRIX_ERROR("Invalid size for solution vector", E_MATRIX_NGPTR);
    } else if (l->row != N || l->col != N
        || u->row != N || u->col != N 
        || p->row != N || p->col != N)
    {
        MATRIX_ERROR("L, U, P matrices must be square and have the same size",
            E_MATRIX_NEQSIZ);
    }

    if (matrix_lu_det(l, u, p) == 0) {
        MATRIX_ERROR("Left matrix is singular", E_MATRIX_SLVFAIL);
    }

    matrix_mul(dst, p, b);
    matrix_forward_subst(dst, l, dst);
    matrix_back_subst(dst, u, dst);
}

void matrix_lu_inv(Matrix *dst, const Matrix *l, const Matrix *u, const Matrix *p) {
    int N = l->row;

    if (l->col != N || u->row != N || u->col != N || p->row != N || p->col != N) {
        MATRIX_ERROR("L, U, P and the result matrices must be square and have the same size",
            E_MATRIX_NEQSIZ);
    }

    Matrix *id = matrix_alloc(N, N);
    matrix_set_identity(id);
    matrix_lu_solve(dst, l, u, p, id);
    matrix_free(id);
}