#include "kinect.hpp"
#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace mik {

PYBIND11_MODULE(kinectpy, m) {
    m.doc() = R"pbdoc(
       Kinect wrapper 
        -----------------------
        .. currentmodule:: kinect_py
        .. autosummary::
           :toctree: _generate
    )pbdoc";
    py::class_<Kinect>(m, "Kinect")
        .def(py::init<>())
        .def("start_recording", &Kinect::start_recording, R"pbdoc(
        Starts recording with Kinect. non-blocking.
    )pbdoc")
        .def("stop_recording", &Kinect::stop_recording, R"pbdoc(
        Stops recording with Kinect. non-blocking.
    )pbdoc");

    m.attr("__version__") = "dev";
}
} // namespace mik