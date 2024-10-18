import os
import sys
from PyQt5.QtWidgets import QApplication, QMainWindow, QLabel, QPushButton, QVBoxLayout, QWidget
from PyQt5.QtGui import QPixmap, QImage
from PyQt5.QtCore import Qt, pyqtSignal, QObject
import numpy as np
import ctypes

# 添加 libs 目录到 sys.path
sys.path.append(os.path.abspath('./libs'))
from mediamanager import PlayController


class MainWindow(QMainWindow):

    def __init__(self):
        super().__init__()

        self.setWindowTitle("PyQt 视频播放器")
        self.setGeometry(100, 100, 800, 600)

        # 初始化 PlayController
        self.playController = PlayController()

        # 创建视频显示区域
        self.m_videoView = QLabel(self)
        self.m_videoView.setAlignment(Qt.AlignCenter)

        # 创建播放按钮
        self.playButton = QPushButton("播放视频", self)
        self.playButton.clicked.connect(self.start_play)

        # 布局
        layout = QVBoxLayout()
        layout.addWidget(self.m_videoView)
        layout.addWidget(self.playButton)

        container = QWidget()
        container.setLayout(layout)
        self.setCentralWidget(container)

        # 设置回调函数
        self.playController.setRenderCallback(self.renderFrameRGB)


    def start_play(self):
        # 固定视频文件路径
        video_file = "./media/aki.mp4"
        self.playController.endPlay()
        self.playController.startPlay(video_file)


    def renderFrameRGB(self, data: bytes, width: int, height: int, aspectRatio):
        try:
            print(f"Received data from C++")
            # 定义数组长度
            length = width * height * 4

            # 使用 ctypes 从地址创建数组
            data_array = (ctypes.c_uint8 * length).from_address(data)

            # 渲染图片
            img = QImage(data_array, width, height, QImage.Format_RGB32)
            pixmap = QPixmap.fromImage(img)
            self.m_videoView.setPixmap(pixmap)
        except Exception as e:
            print(f"Error rendering frame: {e}")


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())
