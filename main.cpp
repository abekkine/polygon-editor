#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <GL/glut.h>

#define MAX_VERTICES 16
#define GRID_SIZE 10.0
#define SCREEN_SIZE 1600

struct grid_point {
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
shape_point shape[MAX_VERTICES];

void render();
void idle();
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void keyboard(unsigned char key, int x, int y);

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

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

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

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

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

void render_panel() {

    glColor4f(0.0, 0.0, 0.0, 0.8);
    glBegin(GL_QUADS);
    glVertex2i(10, 10);
    glVertex2i(110, 10);
    glVertex2i(110, 60);
    glVertex2i(10, 60);
    glEnd();

    glLineWidth(1.0);
    glColor4f(1.0, 1.0, 1.0, 0.5);
    glBegin(GL_LINE_LOOP);
    glVertex2i(10, 10);
    glVertex2i(110, 10);
    glVertex2i(110, 60);
    glVertex2i(10, 60);
    glEnd();

    glColor3f(1.0, 1.0, 1.0);
    char strLine1[256];
    char strLine2[256];
    sprintf(strLine1, "%.2f", cursor_on_grid.x);
    sprintf(strLine2, "%.2f", cursor_on_grid.y);

    glRasterPos2i(20, 30);
    for (int i=0; i<strlen(strLine1); ++i) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, strLine1[i]);

    glRasterPos2i(20, 50);
    for (int i=0; i<strlen(strLine2); ++i) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, strLine2[i]);
}

void render_shape() {

    glLineWidth(3.0);
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_LINE_LOOP);
    for (int i=0; i<MAX_VERTICES; ++i) {
        if (shape[i].valid) {
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
}

void mouse(int button, int state, int x, int y) {
    // TODO
}

void calculate_cursor_on_grid() {

    cursor_on_grid.x = 2.0 * GRID_SIZE * cursor_on_screen.x / SCREEN_SIZE - GRID_SIZE;
    cursor_on_grid.y = -2.0 * GRID_SIZE * cursor_on_screen.y / SCREEN_SIZE + GRID_SIZE; 
}

void motion(int x, int y) {

    cursor_on_screen.x = x;
    cursor_on_screen.y = y;

    calculate_cursor_on_grid();
}

void add_point_to_current_shape() {

    shape[shape_point_index].point = cursor_on_grid;
    shape[shape_point_index].valid = true;
    shape_point_index = (shape_point_index+1) % MAX_VERTICES;
}

void keyboard(unsigned char key, int x, int y) {

	switch(key) {
        case 'a':
            add_point_to_current_shape(); break;
		case 27:
			exit(0);
			break;
	}
}

void idle() {
    // TODO
    usleep(20000);
}

