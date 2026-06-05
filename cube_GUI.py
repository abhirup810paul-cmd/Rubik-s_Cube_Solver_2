import sys
import os
import subprocess
import random
import time
import math
from PyQt5.QtCore import QThread, pyqtSignal, QTimer, Qt
from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout,
                             QHBoxLayout, QPushButton, QLineEdit, QTextEdit, QLabel)
from PyQt5.QtOpenGL import QGLWidget
from OpenGL.GL import *
from OpenGL.GLU import *

# --- COLOR DEFINITIONS (Internal Face Mapping) ---
COLORS = {
    'W': (1.0, 1.0, 1.0),  # UP
    'Y': (1.0, 1.0, 0.0),  # DOWN
    'R': (1.0, 0.0, 0.0),  # FRONT
    'O': (1.0, 0.5, 0.0),  # BACK
    'G': (0.0, 0.6, 0.0),  # LEFT
    'B': (0.0, 0.0, 1.0)   # RIGHT
}


class SolverWorker(QThread):
    """Asynchronously communicates with the compiled C++ backend executable."""
    solution_ready = pyqtSignal(str)

    def __init__(self, scramble_str):
        super().__init__()
        self.scramble_str = scramble_str

    def run(self):
        try:
            executable = "./main_created" if os.name != 'nt' else "main_created.exe"
            process = subprocess.Popen(
                [executable],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1
            )
            stdout, _ = process.communicate(input=f"{self.scramble_str}\n", timeout=None)
            self.solution_ready.emit(stdout.strip())
        except Exception as e:
            self.solution_ready.emit(f"Error connecting to backend: {str(e)}")


class CubeGLWidget(QGLWidget):
    move_finished = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.x_rot = 25
        self.y_rot = -45
        self.last_pos = None
        self.current_anim_move = None
        self.anim_angle = 0
        self.target_angle = 90
        self.animate_direction = 1
        self.rot_axis = (0, 1, 0)
        self.slice_func = lambda x, y, z: False
        self.anim_timer = QTimer(self)
        self.anim_timer.timeout.connect(self.update_animation)
        self.reset_state()

    def reset_state(self):
        self.state = {
            'U': [['W'] * 3 for _ in range(3)],
            'D': [['Y'] * 3 for _ in range(3)],
            'F': [['R'] * 3 for _ in range(3)],
            'B': [['O'] * 3 for _ in range(3)],
            'L': [['G'] * 3 for _ in range(3)],
            'R': [['B'] * 3 for _ in range(3)],
        }

    def initializeGL(self):
        glClearColor(0.2, 0.2, 0.2, 1.0)
        glEnable(GL_DEPTH_TEST)
        glShadeModel(GL_SMOOTH)

    def resizeGL(self, width, height):
        if height == 0:
            height = 1
        glViewport(0, 0, width, height)
        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        gluPerspective(45.0, width / height, 1.0, 100.0)
        glMatrixMode(GL_MODELVIEW)

    def update_animation(self):
        step = 2  # <--- CHANGED FROM 5 TO 2 (Makes the animation 2.5x slower & smoother)
        self.anim_angle += step * self.animate_direction
        if abs(self.anim_angle) >= self.target_angle:
            self.anim_timer.stop()
            self.apply_move_to_state(self.current_anim_move)
            self.current_anim_move = None
            self.anim_angle = 0
            self.updateGL()
            QTimer.singleShot(500, self.move_finished.emit)
        else:
            self.updateGL()

    def start_animating_move(self, move):
        if not move:
            return

        face = move[0]
        self.current_anim_move = move
        self.target_angle = 180 if move.endswith("2") else 90
        direction = -1 if "'" in move else 1

        if face == "U":
            self.rot_axis = (0, 1, 0)
            self.slice_func = lambda x, y, z: y == 1
            self.animate_direction = -direction
        elif face == "D":
            self.rot_axis = (0, 1, 0)
            self.slice_func = lambda x, y, z: y == -1
            self.animate_direction = direction
        elif face == "L":
            self.rot_axis = (1, 0, 0)
            self.slice_func = lambda x, y, z: x == -1
            self.animate_direction = direction
        elif face == "R":
            self.rot_axis = (1, 0, 0)
            self.slice_func = lambda x, y, z: x == 1
            self.animate_direction = -direction
        elif face == "F":
            self.rot_axis = (0, 0, 1)
            self.slice_func = lambda x, y, z: z == 1
            self.animate_direction = -direction
        elif face == "B":
            self.rot_axis = (0, 0, 1)
            self.slice_func = lambda x, y, z: z == -1
            self.animate_direction = direction
        else:
            self.anim_angle = 0
            return

        # --- THESE TWO LINES WERE LIKELY MISSING! ---
        self.anim_angle = 0
        self.anim_timer.start(15) 
        # --------------------------------------------

    def apply_move_to_state(self, move):
        """Correctly manipulates the internal color maps to match physical cube mechanics."""
        def rotate_matrix_cw(m):
            return [list(x) for x in zip(*m[::-1])]

        if not move:
            return

        face = move[0]
        
        # Ground truth: 18 moves map down to base face cycles repeated 1, 2, or 3 times
        iterations = 1
        if move.endswith("'"):
            iterations = 3
        elif move.endswith("2"):
            iterations = 2

        for _ in range(iterations):
            if face == "U":
                self.state['U'] = rotate_matrix_cw(self.state['U'])
                temp = self.state['F'][0][:]
                self.state['F'][0] = self.state['R'][0][:]
                self.state['R'][0] = self.state['B'][0][:]
                self.state['B'][0] = self.state['L'][0][:]
                self.state['L'][0] = temp

            elif face == "D":
                self.state['D'] = rotate_matrix_cw(self.state['D'])
                temp = self.state['F'][2][:]
                self.state['F'][2] = self.state['L'][2][:]
                self.state['L'][2] = self.state['B'][2][:]
                self.state['B'][2] = self.state['R'][2][:]
                self.state['R'][2] = temp

            elif face == "L":
                self.state['L'] = rotate_matrix_cw(self.state['L'])
                temp = [self.state['U'][i][0] for i in range(3)]
                for i in range(3):
                    self.state['U'][i][0] = self.state['B'][2-i][2]
                    self.state['B'][2-i][2] = self.state['D'][i][0]
                    self.state['D'][i][0] = self.state['F'][i][0]
                    self.state['F'][i][0] = temp[i]

            elif face == "R":
                self.state['R'] = rotate_matrix_cw(self.state['R'])
                temp = [self.state['U'][i][2] for i in range(3)]
                for i in range(3):
                    self.state['U'][i][2] = self.state['F'][i][2]
                    self.state['F'][i][2] = self.state['D'][i][2]
                    self.state['D'][i][2] = self.state['B'][2-i][0]
                    self.state['B'][2-i][0] = temp[i]

            elif face == "F":
                self.state['F'] = rotate_matrix_cw(self.state['F'])
                temp = self.state['U'][2][:]
                for i in range(3):
                    self.state['U'][2][i] = self.state['L'][2-i][2]
                    self.state['L'][2-i][2] = self.state['D'][0][2-i]
                    self.state['D'][0][2-i] = self.state['R'][i][0]
                    self.state['R'][i][0] = temp[i]

            elif face == "B":
                self.state['B'] = rotate_matrix_cw(self.state['B'])
                temp = self.state['U'][0][:]
                for i in range(3):
                    self.state['U'][0][i] = self.state['R'][i][2]
                    self.state['R'][i][2] = self.state['D'][2][2-i]
                    self.state['D'][2][2-i] = self.state['L'][2-i][0]
                    self.state['L'][2-i][0] = temp[i]

            

    def paintGL(self):
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glLoadIdentity()
        glTranslatef(0.0, 0.0, -12.0)
        glRotatef(self.x_rot, 1.0, 0.0, 0.0)
        glRotatef(self.y_rot, 0.0, 1.0, 0.0)
        self.draw_cube()

    def draw_cube(self):
        for x in [-1, 0, 1]:
            for y in [-1, 0, 1]:
                for z in [-1, 0, 1]:
                    glPushMatrix()
                    
                    # --- NEW: APPLY PHYSICAL ROTATION TO SPECIFIC SLICES ---
                    if self.current_anim_move and self.slice_func(x, y, z):
                        glRotatef(self.anim_angle, *self.rot_axis)
                    
                    glTranslatef(x * 1.05, y * 1.05, z * 1.05)
                    
                    r_x, r_y, r_z = x + 1, 1 - y, z + 1 
                    # Fetch active colors dynamically (Fixed L and R mirroring!)
                    u_col = COLORS[self.state['U'][r_z][r_x]] if y == 1 else (0.1, 0.1, 0.1)
                    d_col = COLORS[self.state['D'][2-r_z][r_x]] if y == -1 else (0.1, 0.1, 0.1)
                    f_col = COLORS[self.state['F'][r_y][r_x]] if z == 1 else (0.1, 0.1, 0.1)
                    b_col = COLORS[self.state['B'][r_y][2-r_x]] if z == -1 else (0.1, 0.1, 0.1)
                    l_col = COLORS[self.state['L'][r_y][r_z]] if x == -1 else (0.1, 0.1, 0.1)
                    r_col = COLORS[self.state['R'][r_y][2-r_z]] if x == 1 else (0.1, 0.1, 0.1)

                    glBegin(GL_QUADS)
                    glColor3fv(u_col); glVertex3f(0.5, 0.5, -0.5); glVertex3f(-0.5, 0.5, -0.5); glVertex3f(-0.5, 0.5, 0.5); glVertex3f(0.5, 0.5, 0.5)
                    glColor3fv(d_col); glVertex3f(0.5, -0.5, 0.5); glVertex3f(-0.5, -0.5, 0.5); glVertex3f(-0.5, -0.5, -0.5); glVertex3f(0.5, -0.5, -0.5)
                    glColor3fv(f_col); glVertex3f(0.5, 0.5, 0.5); glVertex3f(-0.5, 0.5, 0.5); glVertex3f(-0.5, -0.5, 0.5); glVertex3f(0.5, -0.5, 0.5)
                    glColor3fv(b_col); glVertex3f(0.5, -0.5, -0.5); glVertex3f(-0.5, -0.5, -0.5); glVertex3f(-0.5, 0.5, -0.5); glVertex3f(0.5, 0.5, -0.5)
                    glColor3fv(l_col); glVertex3f(-0.5, 0.5, 0.5); glVertex3f(-0.5, 0.5, -0.5); glVertex3f(-0.5, -0.5, -0.5); glVertex3f(-0.5, -0.5, 0.5)
                    glColor3fv(r_col); glVertex3f(0.5, 0.5, -0.5); glVertex3f(0.5, 0.5, 0.5); glVertex3f(0.5, -0.5, 0.5); glVertex3f(0.5, -0.5, -0.5)
                    glEnd()
                    glPopMatrix()

    def mousePressEvent(self, event):
        self.last_pos = event.pos()

    def mouseMoveEvent(self, event):
        if self.last_pos:
            dx = event.x() - self.last_pos.x()
            dy = event.y() - self.last_pos.y()
            self.x_rot += dy * 0.5
            self.y_rot += dx * 0.5
            self.last_pos = event.pos()
            self.updateGL()



class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("IDA* Rubik's Cube Solver Pipeline")
        self.setGeometry(100, 100, 1000, 700)

        self.scramble_queue = []
        self.solution_queue = []
        
        self.init_ui()
        
        # Timeline playback is handled by the CubeGLWidget animation signal.

    def init_ui(self):
        main_widget = QWidget()
        main_layout = QHBoxLayout()

        # Left side: 3D Viewport
        self.gl_cube = CubeGLWidget(self)
        main_layout.addWidget(self.gl_cube, stretch=3)

        # Right side: Configuration panel
        panel_layout = QVBoxLayout()
        
        panel_layout.addWidget(QLabel("<b>Enter Scramble Sequence (e.g., U L F2 R'):</b>"))
        self.scramble_input = QLineEdit("U L U2 L' F'")
        panel_layout.addWidget(self.scramble_input)



        self.btn_generate = QPushButton("Generate Random Scramble")
        self.btn_generate.setStyleSheet("background-color: #e67e22; color: white; font-weight: bold;")
        self.btn_generate.clicked.connect(self.generate_random_scramble)
        panel_layout.addWidget(self.btn_generate)



        self.btn_scramble = QPushButton("Execute Scramble Step-by-Step")
        self.btn_scramble.clicked.connect(self.start_scramble_playback)
        panel_layout.addWidget(self.btn_scramble)

        self.btn_solve = QPushButton("Query C++ IDA* Solver Engine")
        self.btn_solve.setStyleSheet("background-color: #2b579a; color: white; font-weight: bold;")
        self.btn_solve.clicked.connect(self.request_solution)
        panel_layout.addWidget(self.btn_solve)

        panel_layout.addWidget(QLabel("<b>Solver Engine Output Log:</b>"))
        self.log_output = QTextEdit()
        self.log_output.setReadOnly(True)
        panel_layout.addWidget(self.log_output)

        self.btn_play_solution = QPushButton("Animate Solution Moves")
        self.btn_play_solution.setEnabled(False)
        self.btn_play_solution.clicked.connect(self.start_solution_playback)
        panel_layout.addWidget(self.btn_play_solution)

        panel_layout.addStretch()
        main_layout.addLayout(panel_layout, stretch=1)
        main_widget.setLayout(main_layout)
        self.setCentralWidget(main_widget)

        self.gl_cube.move_finished.connect(self.process_next_playback_move) # Connect the cube's animation completion signal to the playback processor

    def generate_random_scramble(self):
        faces = ['U', 'D', 'F', 'B', 'L', 'R']
        modifiers = ['', "'", '2']
        scramble = []
        last_face = ""

        # Generate exactly 10 moves
        for _ in range(10):
            face = random.choice(faces)
            # Ensure we don't pick the exact same face as the last move
            while face == last_face:
                face = random.choice(faces)
            
            modifier = random.choice(modifiers)
            scramble.append(face + modifier)
            last_face = face

        # Combine into a string and update the UI
        scramble_str = " ".join(scramble)
        self.scramble_input.setText(scramble_str)
        self.log_output.append(f"[*] Generated Random 10-Move Scramble!")

        


    def start_scramble_playback(self):
        moves = self.scramble_input.text().strip().split()
        if not moves or moves == [""]: return
        self.log_output.append(f"[!] Scrambling 3D Model: {' '.join(moves)}")
        self.scramble_queue = moves
        self.solution_queue = []
        self.process_next_playback_move() # <--- Changed from timer-based to immediate processing for scramble steps, since they don't need animation


    def request_solution(self):
        scramble = self.scramble_input.text().strip()
        if not scramble: return

        self.log_output.append("[...] Calling C++ Bitboard Process Pipeline...")
        self.btn_solve.setEnabled(False)
        
        # Start worker thread to communicate with your C++ compiled engine
        self.worker = SolverWorker(scramble)
        self.worker.solution_ready.connect(self.handle_solution)
        self.worker.start()

    def handle_solution(self, raw_output):
        self.btn_solve.setEnabled(True)
        if "Error" in raw_output:
            self.log_output.append(f"[ERROR] {raw_output}")
            return

        self.log_output.append(f"[+] Optimal Solution Found!\nSequence: {raw_output}")
        self.solution_queue = raw_output.strip().split()
        if self.solution_queue and self.solution_queue != [""]:
            self.btn_play_solution.setEnabled(True)

    def start_solution_playback(self):
        if not self.solution_queue: return
        self.log_output.append("[!] Executing solution playback on 3D viewport...")
        self.scramble_queue = []
        self.process_next_playback_move() # <--- Changed from timer-based to immediate processing for solution steps, since the CubeGLWidget now handles animation timing internally for each move, so we just trigger the next move immediately and let the cube's animation system handle the pacing.


    def process_next_playback_move(self):
        current_move = None
        if self.scramble_queue:
            current_move = self.scramble_queue.pop(0)
        elif self.solution_queue:
            current_move = self.solution_queue.pop(0)

        if not current_move:
            self.log_output.append("[+] Sequence complete.")
            self.btn_play_solution.setEnabled(False)
            return

        self.log_output.append(f"Applying Move: {current_move}")
        
        # --- NEW: TELL OPENGL TO START THE PHYSICAL ROTATION ---
        self.gl_cube.start_animating_move(current_move)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())

