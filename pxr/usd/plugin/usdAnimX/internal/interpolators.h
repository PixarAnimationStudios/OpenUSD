#ifndef INTERPOLATORS_H
#define INTERPOLATORS_H

namespace adsk
{
    namespace CurveInterpolators
    {
        double sine(double startX, double startY, double x1, double y1, double x2, double y2, double endX, double endY, double time)
        {
            double t = time;
            double t0 = startX;
            double v0 = startY;
            if (t == t0) return v0;

            double t3 = endX;
            double v3 = y2;
            if (t == t3) return v3;
            double len = t3 - t0; // length of seg

            double ei = x1 - startX;
            double eo = x2 - endX;
            if (ei < 0) ei = 0;
            else if (ei > len) ei = len;
            if (eo < 0) eo = 0;
            else if (eo > len - ei) eo = len - ei;

            double t1 = t0 + ei;
            double t2 = t3 - eo;

            // compute mid slope
            double m = (v3 - v0) / (len - (ei + eo) * (1 - 2 / kPI));

            if (t < t1) // ease in
                return v0 + m * ei * (2 / kPI) * (1 - cos((t - t0)*(kPI / 2) / ei));

            else if (t > t2) // ease out
                return v3 - m * eo * (2 / kPI) * (1 - cos((t3 - t)*(kPI / 2) / eo));

            else // middle (linear)
                return v0 + m * (ei * (2 / kPI) + t - t1);
        }

        double parabolic(double startX, double startY, double x1, double y1, double x2, double y2, double endX, double endY, double time)
        {
            double t = time;
            double t0 = startX;
            double v0 = startY;
            if (t == t0) return v0;

            double t3 = endX;
            double v3 = endY;
            if (t == t3) return v3;
            double len = t3 - t0; // length of seg

            double ei = x1 - startX;
            double eo = x2 - endX;
            if (ei < 0) ei = 0;
            else if (ei > len) ei = len;
            if (eo < 0) eo = 0;
            else if (eo > len - ei) eo = len - ei;

            double t1 = t0 + ei;
            double t2 = t3 - eo;

            // compute mid slope
            double m = (v3 - v0) / (len - 0.5 * (ei + eo));

            if (t < t1) { // ease in
                double dt = t - t0;
                return v0 + m * dt * dt / (2 * ei);
            }
            else if (t < t2) { // middle (linear)
                return v0 + m * (0.5 * ei + t - t1);
            }
            else { // ease out
                double dt = t3 - t;
                return v3 - m * dt * dt / (2 * eo);
            }
        }

        double log(double startX, double startY, double x1, double y1, double x2, double y2, double endX, double endY, double time)
        {
            double t = time;
            double t0 = startX;
            double t3 = endX;
            double len = t3 - t0; // length of seg

            if (t == t0) return startY;
            if (t == t3) return endY;

            double ei = x1 - startX;
            double wi = y1 - startY;
            double eo = x2 - endX;
            double wo = y2 - endY;

            if (ei < 0) ei = 0;
            else if (ei > len) ei = len;
            if (eo < 0) eo = 0;
            else if (eo > len - ei) eo = len - ei;

            if (wo <= 0) { wo = 1; }
            if (wi <= 0) { wi = 1; }
            double r = wo / wi;

            // first compute a sin ease, but over normalized range [0..1]
            double v0 = startY;
            double v3 = endY;
            double t1 = t0 + ei;
            double t2 = t3 - eo;

            // compute midslope
            double m = 1 / (len - (ei + eo) * (1 - 2 / kPI));

            double result;
            if (t < t1) { // ease in
                result = m * ei * (2 / kPI) * (1 - cos((t - t0)*(kPI / 2) / ei));
            }
            else if (t > t2) { // ease out
                result = m * (ei * (2 / kPI) + (t2 - t1)
                    + eo * (2 / kPI) * sin((t - t2)*(kPI / 2) / eo));
            }
            else { // middle (linear)
                result = m * (ei * (2 / kPI) + t - t1);
            }

            // then compose with the exp function and re-normalize
            if (!equivalent(r, 1.0))
                result = (exp(::log(r)*result) - 1) / (r - 1);

            // finally, scale+translate to desired range of [v0..v3]
            return v0 + (v3 - v0) * result;
        }

        double bezier(double startX, double startY, double x1, double y1, double x2, double y2, double endX, double endY, double time)
        {
            return Tbezier::evaluate(startX, startY, x1, y1, x2, y2, endX, endY, time);
        }

        double hermite(double startX, double startY, double x1, double y1, double x2, double y2, double endX, double endY, double time)
        {
            // save the control points
            //
            double fX1 = startX;
            double fY1 = startY;
            double fX2 = x1;
            double fY2 = y1;
            double fX3 = x2;
            double fY3 = y2;
            double fX4 = endX;
            double fY4 = endY;

            // Compute the difference between the 2 keyframes.          
            double dx = fX4 - fX1;
            double dy = fY4 - fY1;

            // Compute the tangent at the start of the curve segment.
            double tan_x = fX2 - fX1;
            double m1 = 0.0;
            if (tan_x != 0.0) {
                m1 = (fY2 - fY1) / tan_x;
            }

            tan_x = fX4 - fX3;
            double m2 = 0.0;
            if (tan_x != 0.0) {
                m2 = (fY4 - fY3) / tan_x;
            }

            double length = 1.0 / (dx * dx);
            double double1 = dx * m1;
            double double2 = dx * m2;
            double fCoeff[4];

            fCoeff[0] = (double1 + double2 - dy - dy) * length / dx;
            fCoeff[1] = (dy + dy + dy - double1 - double1 - double2) * length;
            fCoeff[2] = m1;
            fCoeff[3] = fY1;

            double t = time - fX1;
            double val = t * (t * (t * fCoeff[0] + fCoeff[1]) + fCoeff[2]) + fCoeff[3];
            return (val);
        }
    }
}

#endif