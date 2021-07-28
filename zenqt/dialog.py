import sys
from PySide2.QtWidgets import *
from PySide2.QtCore import *
from PySide2.QtGui import *


class RecordVideoDialog(QDialog):
    def __init__(self, result, frame_count):
        super().__init__()

        self.setWindowTitle('Record screen')    
        self.result = result
        self.frame_count = frame_count
        self.initUI()

    def initUI(self):
        frame_start = QLabel('Frame start:')
        self.frame_start_edit = QSpinBox()
        self.frame_start_edit.setMinimum(0)
        self.frame_start_edit.setMaximum(self.frame_count - 1)
        self.frame_start_edit.setValue(0)


        frame_end = QLabel('Frame end:')
        self.frame_end_edit = QSpinBox()
        self.frame_end_edit.setMinimum(0)
        self.frame_end_edit.setMaximum(self.frame_count - 1)
        self.frame_end_edit.setValue(self.frame_count - 1)

        fps = QLabel('FPS:')
        self.fps_edit = QSpinBox()
        self.fps_edit.setMinimum(1)
        self.fps_edit.setValue(30)


        viewport_width = QLabel('Width:')
        self.viewport_width_eidtor = QLineEdit('1280')

        viewport_height = QLabel('Height:')
        self.viewport_height_eidtor = QLineEdit('720')

        ok_button = QPushButton('OK')
        cancel_button = QPushButton('Cancel')

        ok_button.clicked.connect(self.accept)
        cancel_button.clicked.connect(self.reject)

        
        presets = QLabel('Presets:')
        combo = self.build_combobox()

        grid = QGridLayout()
        grid.setSpacing(10)

        grid.addWidget(frame_start, 1, 0)
        grid.addWidget(self.frame_start_edit, 1, 1)

        grid.addWidget(frame_end, 2, 0)
        grid.addWidget(self.frame_end_edit, 2, 1)

        grid.addWidget(fps, 3, 0)
        grid.addWidget(self.fps_edit, 3, 1)

        grid.addWidget(presets, 4, 0)
        grid.addWidget(combo, 4, 1)

        grid.addWidget(viewport_width, 5, 0)
        grid.addWidget(self.viewport_width_eidtor, 5, 1)

        grid.addWidget(viewport_height, 6, 0)
        grid.addWidget(self.viewport_height_eidtor, 6, 1)

        grid.addWidget(ok_button, 7, 0)
        grid.addWidget(cancel_button, 7, 1)


        self.setLayout(grid) 

    def accept(self):
        r = self.result
        r['frame_start'] = self.frame_start_edit.value()
        r['frame_end'] = self.frame_end_edit.value()
        r['fps'] = self.fps_edit.value()
        r['width'] = int(self.viewport_width_eidtor.text())
        r['height'] = int(self.viewport_height_eidtor.text())
        super().accept()

    def build_combobox(self):
        screen_resolution = {
            '540P': (960, 540),
            '720P': (1280, 720),
            '1080P': (1920, 1080),
            '2K': (2560, 1440),
            '4K': (3840, 2160),
        }
        c = QComboBox()
        c.addItems(screen_resolution.keys())
        def callback(text):
            w, h = screen_resolution[text]
            self.viewport_width_eidtor.setText(str(w))
            self.viewport_height_eidtor.setText(str(h))
        c.textActivated.connect(callback)
        c.setCurrentIndex(1)
        return c

if __name__ == '__main__':
    app = QApplication(sys.argv)
    r = {}
    ex = Example(r)
    ex.open()
    print(r)

    sys.exit(app.exec_())
