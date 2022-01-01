#ifndef TROTATEXYZ_H
#define TROTATEXYZ_H

namespace adsk
{
    struct TrotateXYZ
    {
        double x, y, z;

        TrotateXYZ()
        {}

        TrotateXYZ(const T4DblVector &v) 
            : x(v.x)
            , y(v.y)
            , z(v.z)
        {}

        static TrotateXYZ decompose1(const T4dDblMatrix &m)
        {
            T4DblVector solutions[2];

            if (decompose(m, solutions)) {
                //  If multiple decompositions exist use a heuristic to pick
                //  a solution that minimizes total rotation.
                //
                if (std::abs(solutions[1].x) + std::abs(solutions[1].y) + std::abs(solutions[1].z) <
                    std::abs(solutions[0].x) + std::abs(solutions[0].y) + std::abs(solutions[0].z))
                {
                    //  Choose second decomposition
                    return TrotateXYZ(solutions[1]);
                }
            }
            //  Choose first decomposition
            return TrotateXYZ(solutions[0]);
        }

        static bool decompose(const T4dDblMatrix &m, T4DblVector *solutions)
        {
            bool    multiple = false;
            double  a, b, c;
            double  cosB2 = m[0][0] * m[0][0] + m[0][1] * m[0][1];
            if (cosB2 > kDblEpsilon) 
            {  
                //we check on epsilon on cosB2 instead of sqrt(cosB2) to avoid having extra small value be considered significative
                solutions[0].x = a = atan2( m[1][2], m[2][2]);
                solutions[0].y = b = atan2(-m[0][2], sqrt(cosB2));
                solutions[0].z = c = atan2( m[0][1], m[0][0]);

                solutions[1].x = a + (   lessThan(a,  kPI) ? kPI : -kPI);
                solutions[1].y =     (greaterThan(b, -kPI) ? kPI : -kPI) - b;
                solutions[1].z = c + (   lessThan(c,  kPI) ? kPI : -kPI);
                multiple = true;
            }
            else 
            {
                solutions[0].x = atan2(-m[2][1], m[1][1]);
                solutions[0].y = atan2(-m[0][2], sqrt(cosB2));
                solutions[0].z = 0;
                multiple = false;
            }

            return multiple;
        }
    };
}

#endif