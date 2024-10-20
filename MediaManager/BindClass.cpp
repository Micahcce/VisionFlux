#include <pybind11/pybind11.h>
#include "PlayController.h"
namespace py = pybind11;

PYBIND11_MODULE(mediamanager, m) {
    py::class_<PlayController>(m, "PlayController")
        .def(py::init<>())
        .def("startPlay", &PlayController::startPlay)
        .def("continuePlay", &PlayController::continuePlay)
        .def("pausePlay", &PlayController::pausePlay)
        .def("endPlay", &PlayController::endPlay)
        .def("changePlayProgress", &PlayController::changePlayProgress)
        .def("changePlaySpeed", &PlayController::changePlaySpeed)
        .def("changeVolume", &PlayController::changeVolume)
        .def("streamConvert", &PlayController::streamConvert)
        .def("getMediaDuration", &PlayController::getMediaDuration)
        .def("saveFrameToBmp", &PlayController::saveFrameToBmp)
        .def("timeFormatting", &PlayController::timeFormatting)
        .def("setRenderCallback", &PlayController::setRenderCallback)
        .def("getMediaPlayInfo", &PlayController::getMediaPlayInfo);
}
