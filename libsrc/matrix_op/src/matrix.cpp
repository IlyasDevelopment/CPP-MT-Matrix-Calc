#include "matrix_op/matrix.hpp"
#include "matrix_op/matrix_exception.hpp"

namespace matrix_op {

Matrix operator*(const Matrix& first, const Matrix& another)
{
    if (first.columns_ != another.rows_) [[unlikely]]
    {
        std::stringstream ss;
        ss << "Cannot multiply matrices: (" << first.rows_ << " x " << first.columns_ << ") * ("
           << another.rows_ << " x " << another.columns_ << "): c1 != r2";

        throw MatrixCalcError(ss.str());
    }

    Matrix result(first.rows_, another.columns_);
    for (std::uint32_t r = 0; r < first.rows_; r++)
     for (std::uint32_t c = 0; c < another.columns_; c++)
    {
        float sum = 0;
        for (std::uint32_t i = 0; i < first.columns_; i++)
            sum += first[r][i] * another[i][c];
        result[{r, c}] = sum;
    }

    return result;
}

} // namespace matrix_op
