#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <GL/glut.h>

#define SELECT_DISTANCE_SQ 0.04
#define MAX_SHAPE_POINTS 16
#define GRID_SIZE 10.0
#define SCREEN_SIZE 1600

uint8_t debug_enable_ = 0;

struct grid_point {
    double distance_square(const grid_point& cursor) {
        double dx = x - cursor.x;
        double dy = y - cursor.y;
        return (dx*dx + dy*dy);
    }
    double x;
    double y;
} cursor_on_grid;

struct screen_point {
    int x;
    int y;
} cursor_on_screen;

struct shape_point {
    shape_point() : valid(false) {}
    bool valid;
    grid_point point;
};

int shape_point_index = 0;
shape_point shape[MAX_SHAPE_POINTS];
int selected_point_index = -1;
int move_point_index = -1;

void render();
void idle();
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void keyboard(unsigned char key, int x, int y);
void add_point_to_current_shape();

#define MAX_TEXT_BUFFER 256 
char text_buffer[MAX_TEXT_BUFFER];
void text_print(int x, int y, const char* format, ...) {
    memset(text_buffer, 0, MAX_TEXT_BUFFER);
    va_list args;
    va_start(args, format);
    vsnprintf(text_buffer, MAX_TEXT_BUFFER, format, args);
    va_end(args);

    void* font = GLUT_BITMAP_8_BY_13;

    glRasterPos2i(x, y);
    for (unsigned int i=0; i<sizeof(text_buffer); ++i) {
        glutBitmapCharacter(font, text_buffer[i]);
    }
}

void display(void) {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	render();

	glutSwapBuffers();
	glutPostRedisplay();
}

void reshape(int width, int height) {

	glViewport(0, 0, width, height);
}

void init(void) {

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_MULTISAMPLE);

	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutPassiveMotionFunc(motion);
	glutReshapeFunc(reshape);
	glutIdleFunc(idle);

	glClearColor(0.0, 0.0, 0.0, 0.0);
}

void grid_mode() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-GRID_SIZE, GRID_SIZE, -GRID_SIZE, GRID_SIZE, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
}

void ui_mode() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, SCREEN_SIZE, SCREEN_SIZE, 0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {

    int screen_size = SCREEN_SIZE;

	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);

	glutInitWindowSize(screen_size, screen_size);

	glutInitWindowPosition(10, 10);

	glutCreateWindow("Poly Designer");

	init();

	glutMainLoop();

	return 0;
}

void render_axes() {
    glLineWidth(2.0);
    glColor3f(1.0, 0.0, 0.0);
    glBegin(GL_LINES);
        glVertex2d(-GRID_SIZE, 0.0);
        glVertex2d( GRID_SIZE, 0.0);
        glVertex2d( 0.0, -GRID_SIZE);
        glVertex2d( 0.0,  GRID_SIZE);
    glEnd();
}

void render_grid() {

    glLineWidth(1.0);
    glColor4f(1.0, 0.0, 0.0, 0.4);
    glBegin(GL_LINES);
    for (double s=-GRID_SIZE; s<=GRID_SIZE; s+=1.0) {
        glVertex2d(-GRID_SIZE, s);
        glVertex2d( GRID_SIZE, s);
        glVertex2d( s, -GRID_SIZE);
        glVertex2d( s,  GRID_SIZE);
    }
    glEnd();
}

void render_panel_frame(int top, int left, int width, int height) {

    glPopAttrib();
    glBegin(GL_QUADS);
    glVertex2i(left, top);
    glVertex2i(left + width, top);
    glVertex2i(left + width, top + height);
    glVertex2i(left, top + height);
    glEnd();

    glLineWidth(2.0);
    glColor4f(1.0, 1.0, 1.0, 0.3);
    glBegin(GL_LINE_LOOP);
    glVertex2i(left, top);
    glVertex2i(left + width, top);
    glVertex2i(left + width, top + height);
    glVertex2i(left, top + height);
    glEnd();
}

void render_panel() {

    // Background color
    glColor4f(0.0, 0.0, 0.0, 0.8);
    glPushAttrib(GL_COLOR_BUFFER_BIT);
    render_panel_frame(10, 10, 100, 50);

    glColor3f(1.0, 1.0, 1.0);
    text_print(20, 30, "%.2f", cursor_on_grid.x);
    text_print(20, 50, "%.2f", cursor_on_grid.y);
}

void render_debug_panel() {

    if (debug_enable_ == 0) return;

    // Background color
    glColor4f(0.0, 0.0, 1.0, 0.5);
    glPushAttrib(GL_COLOR_BUFFER_BIT);
    render_panel_frame(SCREEN_SIZE - 110, 10, 400, 100);

    glColor3f(1.0, 1.0, 1.0);
    text_print(20, SCREEN_SIZE - 90, "%d", selected_point_index);
}

void render_shape() {

    glLineWidth(3.0);
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_LINE_LOOP);
    for (int i=0; i<MAX_SHAPE_POINTS; ++i) {
        if (shape[i].valid) {
            glVertex2d(shape[i].point.x, shape[i].point.y);
        }
    }
    glEnd();

    glPointSize(10.0);
    glBegin(GL_POINTS);
    for (int i=0; i<MAX_SHAPE_POINTS; ++i) {
        if (shape[i].valid) {

            if (selected_point_index == i) {
                glColor3f(1.0, 0.0, 1.0);
            }
            else {
                glColor3f(0.0, 0.5, 1.0);
            }

            glVertex2d(shape[i].point.x, shape[i].point.y);
        }
    }
    glEnd();

}

void render() {

    grid_mode();

    render_axes();
    render_grid();
    render_shape();

    ui_mode();

    render_panel();
    render_debug_panel();
}

void mouse(int button, int state, int x, int y) {

    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            if (selected_point_index == -1) {
                add_point_to_current_shape();
            }
            else {
                move_point_index = selected_point_index;
            }
        }
        else if (state == GLUT_UP) {
            move_point_index = -1;
        }
    }
    else if (button == GLUT_RIGHT_BUTTON) {
        if (state == GLUT_DOWN) {
            if (selected_point_index != -1) {
                shape[selected_point_index].valid = false;
            }
        }
    }
}

void calculate_cursor_on_grid() {

    cursor_on_grid.x = 2.0 * GRID_SIZE * cursor_on_screen.x / SCREEN_SIZE - GRID_SIZE;
    cursor_on_grid.y = -2.0 * GRID_SIZE * cursor_on_screen.y / SCREEN_SIZE + GRID_SIZE; 
}

void find_selected_point() {

    selected_point_index = -1;
    for (int i=0; i<MAX_SHAPE_POINTS; ++i) {
        if (shape[i].valid == false) continue;
        double r2 = shape[i].point.distance_square(cursor_on_grid);
        if (r2 <= SELECT_DISTANCE_SQ) {
            selected_point_index = i;
            break;
        }
    }
}

void motion(int x, int y) {

    cursor_on_screen.x = x;
    cursor_on_screen.y = y;

    calculate_cursor_on_grid();

    find_selected_point();

    if (move_point_index != -1) {
        shape[move_point_index].point = cursor_on_grid;
    }
}

void add_point_to_current_shape() {

    shape[shape_point_index].point = cursor_on_grid;
    shape[shape_point_index].valid = true;
    shape_point_index = (shape_point_index+1) % MAX_SHAPE_POINTS;
}

void keyboard(unsigned char key, int x, int y) {

	switch(key) {
        case 'd':
            debug_enable_ ^= 1; break;
		case 27:
			exit(0);
			break;
	}
}

void idle() {
    // TODO
    usleep(20000);
}

