#ifndef T4DBLMATRIX_H
#define T4DBLMATRIX_H

namespace adsk
{
    struct T4dDblMatrix
    {
        double matrix[4][4];

        inline double * operator [] (unsigned int i)
        {
            return &matrix[i][0];
        }

        inline const double * operator [] (unsigned int i) const
        {
            return &matrix[i][0];
        }

        inline double & operator() (short i, short j)
        {
            return matrix[i][j];
        }
    };
}

#endif