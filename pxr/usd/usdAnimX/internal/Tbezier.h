#ifndef TBEZIER_H
#define TBEZIER_H

namespace adsk
{
	class Tbezier
	{
	private:
		static void bezierToPower(
			double a1, double b1, double c1, double d1,
			double &a2, double &b2, double &c2, double &d2)
		{
			double a = b1 - a1;
			double b = c1 - b1;
			double c = d1 - c1;
			double d = b - a;
			a2 = c - b - d;
			b2 = d + d + d;
			c2 = a + a + a;
			d2 = a1;
		}

		static void checkMonotonic(double &x1, double &x2, bool keepFirst)
		{
			// We want a control vector of [ 0 x1 (1-x2) 1 ] since this provides
			// more symmetry. (This yields better equations and constrains x1 and x2
			// to be positive.)
			//
			x2 = 1.0 - x2;

			// x1 and x2 must always be positive
			if (x1 < 0.0)
				x1 = 0.0;
			if (x2 < 0.0)
				x2 = 0.0;

			// If x1 or x2 are greater than 1.0, then they must be inside the
			// ellipse (x1^2 + x2^2 - 2(x1 +x2) + x1*x2 + 1).
			// x1 and x2 are invalid if x1^2 + x2^2 - 2(x1 +x2) + x1*x2 + 1 > 0.0
			//
			//
			if (x1 > 1.0 || x2 > 1.0)
			{
				double	d = x1 * (x1 - 2.0 + x2) + x2 * (x2 - 2.0) + 1.0;

				if (d + kDblEpsilon > 0.0)
				{
					if (keepFirst)
					{
						constrainInsideBounds(x1, x2);
					}
					else
					{
						constrainInsideBounds(x2, x1);
					}
				}
			}

			// we change the control vector back to [ 0 x1 x2 1 ]
			//
			x2 = 1.0 - x2;
		}

		static void constrainInsideBounds(double &x1, double &x2)
		{
			// (x1^2 + x2^2 - 2(x1 +x2) + x1*x2 + 1)
			//   = x2^2 + (x1 - 2)*x1 + (x1^2 - 2*x1 + 1)
			// Therefore, we solve for x2.

			static const double kFourThirds = 4.0 / 3.0;
			static const double kOneThird = 1.0 / 3.0;

			double b, c;
			if (x1 + kDblEpsilon < kFourThirds)
			{
				b = x1 - 2.0;
				c = x1 - 1.0;
				double	discr = sqrt(b * b - 4 * c * c);
				double	root = (-b + discr) * 0.5;
				if (x2 + kDblEpsilon > root)
				{
					x2 = root - kDblEpsilon;
				}
				else {
					root = (-b - discr) * 0.5;
					if (x2 < root + kDblEpsilon)
						x2 = root + kDblEpsilon;
				}
			}
			else {
				x1 = kFourThirds - kDblEpsilon;
				x2 = kOneThird - kDblEpsilon;
			}
		}

		static int quadraticRoots(double a, double b, double c, double & r1, double & r2)
		{
			unsigned int numRoots = 0;
			if (a == 0.0)
			{
				if (b != 0.0)
				{
					numRoots = 1;
					r1 = -c / b;
				}
			}
			else {
				double discriminant = b * b - 4 * a * c;
				if (discriminant < 0.0)
				{
					// Ignore the roots.. Actually, this case should not arise at all
					// if the curve is monotonically increasing..
					//
					//			r1 = r2 = 0.0;
				}
				else {
					a *= 2;
					if (discriminant == 0.0)
					{
						numRoots = 1;
						r1 = -b / a;
					}
					else {
						numRoots = 2;
						discriminant = sqrt(discriminant);
						r1 = (-b - discriminant) / a;
						r2 = (-b + discriminant) / a;
					}
				}
			}
			return numRoots;
		}

	public:
		static double evaluate(double startX, double startY, double x1, double y1, double x2, double y2, double endX, double endY, double time)
		{
			double rangeX = endX - startX;
			if (rangeX == 0.0) {
				return 0.0;
			}
			double	dx1 = x1 - startX;
			double	dx2 = x2 - startX;

			// normalize X control values
			//
			double	nX1 = dx1 / rangeX;
			double	nX2 = dx2 / rangeX;

			// if all 4 CVs equally spaced, polynomial will be linear
			bool fLinear = true;
			bool keepFirst = true;
			static const double oneThird = 1.0 / 3.0;
			static const double twoThirds = 2.0 / 3.0;
			if (equivalent(nX1, oneThird, 1e-6) && equivalent(nX2, twoThirds, 1e-6)) {
				fLinear = true;
			}
			else {
				fLinear = false;
			}

			// save the orig normalized control values
			//
			double oldX1 = nX1;
			double oldX2 = nX2;

			// check the inside control values yield a monotonic function.
			// if they don't correct them with preference given to one of them.
			//
			// Most of the time we are monotonic, so do some simple checks first
			//
			if (nX1 < 0.0) nX1 = 0.0;
			if (nX2 > 1.0) nX2 = 1.0;
			if ((nX1 > 1.0) || (nX2 < 0.0)) {
				checkMonotonic(nX1, nX2, keepFirst);
			}

			// compute the new control points
			//
			if (nX1 != oldX1)
			{
				x1 = startX + nX1 * rangeX;
				if (!equivalent(oldX1, 0.0)) {
					y1 = startY + (y1 - startY) * nX1 / oldX1;
				}
			}
			if (nX2 != oldX2)
			{
				x2 = startX + nX2 * rangeX;
				if (!equivalent(oldX2, 1.0)) {
					y2 = endY - (endY - y2) * (1.0 - nX2) / (1.0 - oldX2);
				}
			}

			// save the control points
			//
			double fX1 = startX;
			double fY1 = startY;
			double fY2 = y1;
			double fY3 = y2;
			double fX4 = endX;
			double fY4 = endY;

			double fPolyX[4];
			double fPolyY[4];

			// convert Tbezier basis to power basis
			//
			bezierToPower(0.0, nX1, nX2, 1.0,
				fPolyX[3], fPolyX[2], fPolyX[1], fPolyX[0]);

			bezierToPower(fY1, fY2, fY3, fY4,
				fPolyY[3], fPolyY[2], fPolyY[1], fPolyY[0]);



			double t;
			double	s;

			if (equivalent(time, fX1))
			{
				s = 0.0;
			}
			else if (equivalent(time, fX4))
			{
				s = 1.0;
			}
			else
			{
				s = (time - fX1) / (fX4 - fX1);
			}

			// if linear, t=s
			if (fLinear)
			{
				t = s;
			}
			else
			{
				// temporarily make X(t) = X(t) - s
				//
				double poly[4];
				poly[3] = fPolyX[3];
				poly[2] = fPolyX[2];
				poly[1] = fPolyX[1];
				poly[0] = fPolyX[0] - s;


				// find the roots of the polynomial.  We are looking for only one
				// in the interval [0, 1]
				//
				double	roots[5];
				int	numRoots = nurbs::polyZeroes(poly, 3, 0.0, true, 1.0, true, roots);
				if (numRoots == 1)
				{
					t = roots[0];
				}
				else {
					t = 0.0;
				}
			}

			return (t * (t * (t * fPolyY[3] + fPolyY[2]) + fPolyY[1]) + fPolyY[0]);
		}
	};
}

#endif