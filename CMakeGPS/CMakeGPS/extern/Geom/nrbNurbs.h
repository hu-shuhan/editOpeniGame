int nrbFindSpan(int n, int p, double u, double *knot);

void nrbAllBsplineBasisFuns(int i, double u, int p, double *knot, double *N);

void nrbAllBsplineBasisFunsDers(int i, double u, int p, double *knot, int nd, double **ders);

double** init2DArray(int x, int y);

void free2Darray(double **array, int x);




