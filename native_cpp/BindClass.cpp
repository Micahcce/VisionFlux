#include <pybind11/pybind11.h>
#include "PlayController.h"
namespace py = pybind11;

PYBIND11_MODULE(visionflux, m) {
    py::class_<PlayController>(m, "PlayController")
        .def(py::init<>())  // 绑定构造函数

        // 绑定所有公有函数
        .def("startPlay", &PlayController::startPlay, py::arg("filePath"))
        .def("resumePlay", &PlayController::resumePlay)
        .def("pausePlay", &PlayController::pausePlay)
        .def("endPlay", &PlayController::endPlay)
        .def("changePlayProgress", &PlayController::changePlayProgress, py::arg("timeSecs"))
        .def("changePlaySpeed", &PlayController::changePlaySpeed, py::arg("speedFactor"))
        .def("changeVolume", &PlayController::changeVolume, py::arg("volume"))
        .def("changeFrameSize", &PlayController::changeFrameSize, py::arg("width"), py::arg("height"), py::arg("uniformScale"))
        .def("setSafeCudaAccelerate", &PlayController::setSafeCudaAccelerate, py::arg("state"))
        .def("streamConvert", &PlayController::streamConvert, py::arg("inputStreamUrl"), py::arg("outputStreamUrl"))
        .def("getMediaDuration", &PlayController::getMediaDuration, py::arg("filePath"))
        .def("getPlayProgress", &PlayController::getPlayProgress)
        .def("saveFrameToBmp", &PlayController::saveFrameToBmp, py::arg("filePath"), py::arg("outputPath"), py::arg("sec"))
        .def("timeFormatting", &PlayController::timeFormatting, py::arg("secs"))
        .def("setRenderCallback", &PlayController::setRenderCallback, py::arg("callback"))

        // 提供访问 m_mediaInfo 的接口
        .def("getMediaPlayInfo", &PlayController::getMediaPlayInfo, py::return_value_policy::reference);  // 返回指针
}


//pybind11_add_module() 决定生成的输出文件名
//PYBIND11_MODULE() 决定 Python 中的导入名

