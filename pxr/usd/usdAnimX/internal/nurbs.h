#ifndef NURBS_H
#define NURBS_H

#include <math.h>
#include <memory>

namespace adsk
{
	namespace nurbs
	{
		double initTolerance()
		{
			double one = 1.0, half = 0.5, sum = 0.0, tolerance = 1.0;
			do {
				tolerance = tolerance * half;
				sum = tolerance + one;
			} while (sum > one);
			return tolerance * 2.0;
		}

		const double epsilon = initTolerance();

		struct  Polynomial
		{
			double  *p;
			int     deg;
		};

		inline double horner1(double P[], int deg, double s)
		{
			double h;

			h = P[deg];
			while (--deg >= 0) h = s*h + P[deg];

			return (h);
		}

		double zeroin2(
			double a, double b, double fa, double fb,
			double tol, Polynomial* pars)
		{
			int    test;
			double c, d, e, fc, del, m, p, q, r, s;

			/* start iteration */
		label1:
			c = a;  fc = fa;  d = b - a;  e = d;
		label2:
			if (std::abs(fc) < std::abs(fb))
			{
				a = b;   b = c;   c = a;   fa = fb;   fb = fc;   fc = fa;
			}

			/* convergence test */
			del = 2.0 * epsilon * std::abs(b) + 0.5*tol;
			m = 0.5 * (c - b);
			test = ((std::abs(m) > del) && (fb != 0.0));
			if (test)
			{
				if ((std::abs(e) < del) || (std::abs(fa) <= std::abs(fb)))
				{
					/* bisection */
					d = m;  e = d;
				}
				else
				{
					s = fb / fa;
					if (a == c)
					{
						/* linear interpolation */
						p = 2.0*m*s;    q = 1.0 - s;
					}
					else
					{
						/* inverse quadratic interpolation */
						q = fa / fc;
						r = fb / fc;
						p = s*(2.0*m*q*(q - r) - (b - a)*(r - 1.0));
						q = (q - 1.0)*(r - 1.0)*(s - 1.0);
					}
					/* adjust the sign */
					if (p > 0.0) q = -q;  else p = -p;
					/* check if interpolation is acceptable */
					s = e;   e = d;
					if ((2.0*p < 3.0*m*q - std::abs(del*q)) && (p < std::abs(0.5*s*q)))
					{
						d = p / q;
					}
					else
					{
						d = m;     e = d;
					}
				}
				/* complete step */
				a = b;     fa = fb;
				if (std::abs(d) > del)   b += d;
				else if (m > 0.0) b += del;  else b -= del;
				fb = horner1(pars->p, pars->deg, b);
				if (fb*(fc / std::abs(fc)) > 0.0)
					goto label1;
				else
					goto label2;
			}

			return (b);
		}

		double zeroin(double a, double b, double tol, Polynomial* pars)
		{
			double fa = horner1(pars->p, pars->deg, a);
			if (std::abs(fa) < epsilon) return(a);

			double fb = horner1(pars->p, pars->deg, b);
			if (std::abs(fb) < epsilon) return(b);

			return (zeroin2(a, b, fa, fb, tol, pars));
		}


		int polyZeroes(double Poly[],
			int deg,
			double a, int a_closed,
			double b, int b_closed,
			double Roots[])
		{
			int           al, left_ok, right_ok, nr, ndr, skip;
			double        e, f, s, pe, ps, tol, *p, p_x[22], *d, d_x[22], *dr, dr_x[22];
			Polynomial ply;

			e = pe = 0.0;

			// f = v_len1(Poly, deg+1);
			//
			f = 0.0;
			int i;
			for (i = 0; i < deg + 1; ++i)
			{
				f += std::abs(Poly[i]);
			}
			tol = (std::abs(a) + std::abs(b))*(deg + 1)*epsilon;

			/* Zero polynomial to tolerance? */
			if (f <= tol)  return(-1);

			/* Check to see if work arrays large enough */
			if (deg <= 20)
			{
				p = p_x;
				d = d_x;
				dr = dr_x;
				al = 0;
			}
			else
			{
				al = deg + 2;
				p = new double[al];
				d = new double[al];
				dr = new double[al];
			}

			// normalize the polynomial ( by absolute values) 
			// V_aA( 1.0/f, Poly, p, deg+1 );
			//
			for (i = 0; i < deg + 1; ++i)
			{
				p[i] = 1.0 / f * Poly[i];
			}

			/* determine true degree */
			while (std::abs(p[deg]) < tol) deg--;

			/* Identically zero poly already caught so constant fn != 0 */
			nr = 0;
			if (deg == 0) goto done;


			/* check for linear case */
			if (deg == 1)
			{
				Roots[0] = -p[0] / p[1];
				left_ok = (a_closed) ? (a < Roots[0] + tol) : (a < Roots[0] - tol);
				right_ok = (b_closed) ? (b > Roots[0] - tol) : (b > Roots[0] + tol);
				nr = (left_ok && right_ok) ? 1 : 0;
				if (nr)
				{
					if (a_closed && Roots[0] < a) Roots[0] = a;
					else if (b_closed && Roots[0] > b) Roots[0] = b;
				}
				goto done;
			}

			/* handle non-linear case */
			else
			{
				ply.p = p;  ply.deg = deg;

				/* compute derivative */
				for (i = 1; i <= deg; i++) d[i - 1] = i*p[i];

				/* find roots of derivative */
				ndr = polyZeroes(d, deg - 1, a, 0, b, 0, dr);
				if (ndr == -1)
				{
					nr = 0;
					goto done;
				}

				/* find roots between roots of the derivative */
				for (i = skip = 0; i <= ndr; i++)
				{
					if (nr > deg) goto done;
					if (i == 0)
					{
						s = a; ps = horner1(p, deg, s);
						if (std::abs(ps) <= tol && a_closed) Roots[nr++] = a;
					}
					else
					{
						s = e;
						ps = pe;
					}
					if (i == ndr)
					{
						e = b;
						skip = 0;
					}
					else
						e = dr[i];
					pe = horner1(p, deg, e);
					if (skip) skip = 0;
					else
					{
						if (std::abs(pe) < tol)
						{
							if (i != ndr || b_closed)
							{
								Roots[nr++] = e;
								skip = 1;
							}
						}
						else if ((ps < 0 && pe>0) || (ps > 0 && pe < 0))
						{
							Roots[nr++] = zeroin(s, e, 0.0, &ply);
							if ((nr > 1) && Roots[nr - 2] >= Roots[nr - 1] - tol)
							{
								Roots[nr - 2] = (Roots[nr - 2] + Roots[nr - 1]) * 0.5;
								nr--;
							}
						}
					}
				}
			}
		done:
			if (al)
			{
				delete[] dr;
				delete[] d;
				delete[] p;
			}

			return nr;
		}
	}
}

#endif