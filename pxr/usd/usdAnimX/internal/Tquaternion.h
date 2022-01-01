#ifndef TQUATERNION_H   
#define TQUATERNION_H

namespace adsk
{
    struct Tquaternion;

    double dot(const Tquaternion &q0, const Tquaternion &q1);
    Tquaternion operator* (double s, const Tquaternion& rightQ);

    struct Tquaternion
    {
        double x = 0.0, y = 0.0, z = 0.0, w = 1.0;

        Tquaternion()
            : x(0.0)
            , y(0.0)
            , z(0.0)
            , w(1.0)
        {}

        Tquaternion(double a, double b, double c, double d)
            : x(a)
            , y(b)
            , z(c)
            , w(d)
        {}

        inline Tquaternion(Quaternion q) : x(q.x), y(q.y), z(q.z), w(q.w) {}

        inline operator Quaternion()
        {
            return Quaternion{ x, y, z, w };
        }

        inline Tquaternion& negateIt()
        {
            w = -w; x = -x; y = -y; z = -z; 
            return *this;
        }

        inline Tquaternion scale(double s) const
        {
            return Tquaternion(s*x, s*y, s*z, s*w);
        }

        inline Tquaternion& scaleIt(double s)
        {
            w = s * w; x = s * x; y = s * y; z = s * z; 
            return *this;
        }

        inline Tquaternion operator-() const
        {
            Tquaternion qNegated(*this); 
            return qNegated.negateIt();
        }

        inline Tquaternion operator-(const Tquaternion &r) const
        {
            return Tquaternion(x - r.x, y - r.y, z - r.z, w - r.w);
        }

        inline Tquaternion operator+(const Tquaternion &r) const
        {
            return Tquaternion(x + r.x, y + r.y, z + r.z, w + r.w);
        }

        inline Tquaternion& operator *= (const Tquaternion &rhs)
        {
            *this = *this * rhs; 
            return *this;
        }

        inline Tquaternion & conjugateIt()
        {
            x = -x; y = -y; z = -z; 
            return *this;
        }

        inline Tquaternion conjugate() const
        {
            Tquaternion qCnj(*this); 
            return qCnj.conjugateIt();
        }

        inline static double computeW(double x, double y, double z)
        {
            return sqrt(1.0 - x*x - y*y - z*z);
        }

        Tquaternion inverse() const
        {
            Tquaternion qInv(*this);
            return qInv.invertIt();
        }

        Tquaternion& invertIt()
        {
            double norminv = 1.0 / (w*w + x*x + y*y + z*z);

            w = w * norminv;
            x = -x * norminv;
            y = -y * norminv;
            z = -z * norminv;
            return *this;
        }

        void convertToMatrix(T4dDblMatrix& tm) const
        {
            //  Common subexpressions
            double ww = w*w, xx = x*x, yy = y*y, zz = z*z;
            double s = 2.0 / (ww + xx + yy + zz);
            double xy = x*y, xz = x*z, yz = y*z, wx = w*x, wy = w*y, wz = w*z;

            // Use this if multiply vectors on the left (pre-multipy).
            // This just the transposed matrix of the one above.
            tm.matrix[0][0] = 1.0 - s * (yy + zz);
            tm.matrix[1][0] = s * (xy - wz);
            tm.matrix[2][0] = s * (xz + wy);
            tm.matrix[3][0] = 0.0;
            tm.matrix[0][1] = s * (xy + wz);
            tm.matrix[1][1] = 1.0 - s * (xx + zz);
            tm.matrix[2][1] = s * (yz - wx);
            tm.matrix[3][1] = 0.0;
            tm.matrix[0][2] = s * (xz - wy);
            tm.matrix[1][2] = s * (yz + wx);
            tm.matrix[2][2] = 1.0 - s * (xx + yy);
            tm.matrix[3][2] = 0.0;
            tm.matrix[0][3] = 0.0;
            tm.matrix[1][3] = 0.0;
            tm.matrix[2][3] = 0.0;
            tm.matrix[3][3] = 1.0;
        }

        TrotateXYZ convertToEulerAngles() const
        {
            //  Generate rotation matrix from quaternion and decompose matrix
            T4dDblMatrix m;
            convertToMatrix(m);
            return TrotateXYZ::decompose1(m);
        }

        Tquaternion operator*(const Tquaternion& rhs) const
        {
            Tquaternion result;
            result.w = rhs.w * w - (rhs.x * x + rhs.y * y + rhs.z * z);
            result.x = rhs.w * x + rhs.x * w + rhs.y * z - rhs.z * y;
            result.y = rhs.w * y + rhs.y * w + rhs.z * x - rhs.x * z;
            result.z = rhs.w * z + rhs.z * w + rhs.x * y - rhs.y * x;
            return result;
        }

        Tquaternion& normalizeIt()
        {
            double lenSqr = w*w + x*x + y*y + z*z;
            if (lenSqr <= kDblEpsilonSqr) {
                w = 1.0;
                x = 0.0;
                y = 0.0;
                z = 0.0;
            }
            else if (!equivalent(lenSqr, 1.0, 2.0*kDblEpsilon)) 
            {
                double length = 1.0 / sqrt(lenSqr);
                w = w*length;
                x = x*length;
                y = y*length;
                z = z*length;
            }
            return *this;
        }

        Tquaternion normalize() const
        {
            Tquaternion qNorm(*this); 
            return qNorm.normalizeIt();
        }

        Tquaternion log() const
        {
            Tquaternion qLog;
            double scale = sqrt(x*x + y*y + z*z);
            double theta = atan2(scale, w);
            if (scale > 0.0)
                scale = theta / scale;
            qLog.w = 0.0;
            qLog.x = scale * x;
            qLog.y = scale * y;
            qLog.z = scale * z;
            return qLog;
        }

        Tquaternion exp() const
        {
            Tquaternion qExp;
            double theta = sqrt(x*x + y*y + z*z);
            double scale = 1.0;
            if (theta > kDblEpsilon) scale = sin(theta) / theta;
            qExp.w = cos(theta);
            qExp.x = scale * x;
            qExp.y = scale * y;
            qExp.z = scale * z;
            return qExp;
        }


        Tquaternion pow(double scalar) const
        {
            return (scalar * log()).exp();
        }
    };

    Tquaternion doubleImpl(const Tquaternion &q0, const Tquaternion &q1)
    {
        return q1.scale(2.0 * dot(q0, q1)) - q0;
    }

    Tquaternion slerp(const Tquaternion& p, const Tquaternion& q, double t)
    {
        Tquaternion tmp(q);
        double cosOmega = dot(p, q);

        // Always take the shortest path
        if (cosOmega < 0)
        {
            cosOmega = -cosOmega;
            tmp = -tmp;
        }

        // Standard case slerp
        if (cosOmega < 1.0 - kSlerpThreshold)
        {
            const double omega = acos(clamp(cosOmega, -1.0, 1.0));
            const double sinOmega = sin(omega);
            const double s1 = sin((1.0 - t) * omega) / sinOmega;
            const double s2 = sin(t * omega) / sinOmega;

            return s1 * p + s2 * tmp;
        }

        // Otherwise p and q are very close, fallback to linear interpolation
        return (p + t * (tmp - p)).normalize();
    }

    Tquaternion bezierPt(const Tquaternion &qPrev,
        const Tquaternion &q,
        const Tquaternion &qNext,
        const bool findNextCtrlPt)
    {
        // Calculate next ctrl point ( a ) relative to q
        Tquaternion ctrlPt = (doubleImpl(qPrev, q) + qNext).normalize();

        // Calculate previous ctrl point ( b-1 ) relative to q
        if (!findNextCtrlPt)
            ctrlPt = doubleImpl(ctrlPt, q);

        return slerp(q, ctrlPt, kOneThird);
    }

    Tquaternion bezier(const Tquaternion &p,
        const Tquaternion &a,
        const Tquaternion &b,
        const Tquaternion &q,
        const double t)
    {
        Tquaternion tmp = slerp(a, b, t);
        return slerp(slerp(slerp(p, a, t), tmp, t),
            slerp(tmp, slerp(b, q, t), t),
            t);
    }

    inline Tquaternion operator* (double s, const Tquaternion& rightQ)
    {
        Tquaternion qScale(rightQ); return qScale.scaleIt(s);
    }

    double dot(const Tquaternion &q0, const Tquaternion &q1)
    {
        return  q0.x*q1.x +
            q0.y*q1.y +
            q0.z*q1.z +
            q0.w*q1.w;
    }
}

#endif