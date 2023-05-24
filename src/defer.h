#ifndef KINETIC_BUILD_DEFER_H
#define KINETIC_BUILD_DEFER_H

#define concat_nx(a, b) a##b
#define concat(a, b) concat_nx(a, b)

#define macro_var(name) concat(name, __LINE__)

#define defer(start, end) for (int macro_var(_i_) = (start, 0); !macro_var(_i_); (macro_var(_i_) += 1), end)

#endif // KINETIC_BUILD_DEFER_H
