#ifndef MATRIX_H
#define MATRIX_H

#include <stdlib.h>

typedef struct Matrix Matrix;

struct Matrix {
  int row;
  int col;
  double *data;
};

#define MATRIX_AT(a, r, c) ( (a)->data[(r) * (a)->col + c] )

/* == error messages == */

enum {
  E_MATRIX_SUCCESS,
  E_MATRIX_NGPTR,
  E_MATRIX_NGSIZ,
  E_MATRIX_NEQSIZ,
  E_MATRIX_NALLOC,
  E_MATRIX_OUTRANGE,
  E_MATRIX_NSQR,
  E_MATRIX_FACFAIL,
  E_MATRIX_SLVFAIL,
  E_MATRIX_DIVZERO
};

const char *matrix_errname(int err);

#define MATRIX_ERROR(reason, err) do { \
  fprintf(stderr, "ERROR: %s: in %s at line %d\n%s", \
          matrix_errname(err), __FILE__, __LINE__, (reason)); \
  fflush(stdout); \
  abort(); \
} while (0)


Matrix *matrix_alloc(size_t row, size_t col);

void matrix_free(Matrix *mat);

void matrix_view(const Matrix *mat);

void matrix_set(Matrix *mat, size_t i, size_t j, double val);

void matrix_from_array(Matrix *mat, const double *arr);

void matrix_copy(Matrix *dst, const Matrix *src);

void matrix_set_all(Matrix *mat, double val);

void matrix_set_identity(Matrix *mat);

double matrix_get(const Matrix *mat, size_t i, size_t j);

double *matrix_get_ptr(const Matrix *mat, size_t i, size_t j);

const double *matrix_get_const_ptr(const Matrix *mat, size_t i, size_t j);

void matrix_add(Matrix *dst, const Matrix *rhs);

void matrix_sub(Matrix *dst, const Matrix *rhs);

void matrix_valmul(Matrix *dst, const Matrix *rhs);

void matrix_valdiv(Matrix *dst, const Matrix *rhs);

void matrix_mul(Matrix *dst, const Matrix *lhs, const Matrix *rhs);

void matrix_transpose(Matrix *dst, const Matrix *src);

void value_swap(double *a, double *b);

void matrix_row_swap(Matrix *mat, size_t u, size_t v);

void matrix_row_scale(Matrix *mat, size_t u, double scalar);

void matrix_add_row_multiple(Matrix *mat, size_t u, size_t v, double scalar);

int matrix_find_nonzero_row(const Matrix *mat, size_t cur_col, size_t st_row);

void matrix_gauss_elim(Matrix *dst, const Matrix *src);

void matrix_gaussjordan_elim(Matrix *dst, const Matrix *src);

void matrix_back_subst(Matrix *dst, const Matrix *a, const Matrix *b);

void matrix_forward_subst(Matrix *dst, const Matrix *a, const Matrix *b);

void matrix_solve(Matrix *dst, const Matrix *a, const Matrix *b);

double matrix_det(const Matrix *mat);

void matrix_inv(Matrix *dst, const Matrix *src);

void matrix_lu_dcmp(Matrix *l, Matrix *u, Matrix *p, const Matrix *src);

void matrix_lu_solve(Matrix *dst, const Matrix *l, const Matrix *u, const Matrix *p,
                     const Matrix *b);

double matrix_lu_det(const Matrix *l, const Matrix *u, const Matrix *p);

void matrix_lu_inv(Matrix *dst, const Matrix *l, const Matrix *u, const Matrix *p);

#endif /* MATRIX_H */