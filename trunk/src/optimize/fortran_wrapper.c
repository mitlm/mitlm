
#define setulb_f77 F77_FUNC (setulb, SETULB)

void setulb_f77(int *n, int *m, double *x, double *l, double *u, int *nbd,
		double *f, double *g, double *factr, double *pgtol,
		double *wa, int *iwa, char *task, int *iprint,
		char *csave, int *lsave, int *isave, double *dsave);


#define lbfgs_f77 F77_FUNC (lbfgs, LBFGS)

void lbfgs_f77(int *n, int *m, double *x, double *f, double *g,
	       int *diagco, double *diag, int *iprint,
	       double *eps, double *xtol, double *w, int *iflag);


void mitlm_setulb(int *n, int *m, double *x, double *l, double *u, int *nbd,
		  double *f, double *g, double *factr, double *pgtol,
		  double *wa, int *iwa, char *task, int *iprint,
		  char *csave, int *lsave, int *isave, double *dsave)
{
	setulb_f77(n, m, x, l, u, nbd, f, g, factr, pgtol, wa,
			 iwa, task, iprint,csave, lsave, isave, dsave);
}

void mitlm_lbfgs(int *n, int *m, double *x, double *f, double *g,
		 int *diagco, double *diag, int *iprint,
		 double *eps, double *xtol, double *w, int *iflag)
{
	lbfgs_f77(n, m, x, f, g, diagco, diag, iprint, eps,
			xtol, w, iflag);
}

