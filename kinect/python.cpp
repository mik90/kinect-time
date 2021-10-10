#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace mik {

int add(int i, int j) { return i + j; }

PYBIND11_MODULE(kinectpy, m) {
    m.doc() = R"pbdoc(
       Kinect wrapper 
        -----------------------
        .. currentmodule:: kinect_py
        .. autosummary::
           :toctree: _generate
           add
           subtract
    )pbdoc";

    m.def("add", &add, R"pbdoc(
        Add two numbers
        Some other explanation about the add function.
    )pbdoc");

    m.def(
        "subtract", [](int i, int j) { return i - j; }, R"pbdoc(
        Subtract two numbers
        Some other explanation about the subtract function.
    )pbdoc");

    m.attr("__version__") = "dev";
}
} // namespace mik