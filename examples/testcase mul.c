int piecewise_func(int x) {
    int result = 0;

    if (x < 3) {
        result = (x - 1) * (x - 1) + 1;
    } else {
        result = -x + 8;
    }

    return result;
}
