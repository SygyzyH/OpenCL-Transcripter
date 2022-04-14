#include "matio.h"
#include "mat.h"

int openmat(const char *name) {
    MATFile *m = matOpen(name, "r");
    if (m == NULL) return MATOPEN_ERR;
}