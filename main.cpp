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
#define MAX_SHAPES 16

const uint8_t kMaxGridScaleIndex = 4;
uint8_t grid_scale_index_ = 0;
double grid_scale_factors_[kMaxGridScaleIndex] = {
    1.0, 0.5, 0.2, 2.0
};

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

int shape_index_ = 0;
int shape_point_index = 0;
shape_point shape[MAX_SHAPES][MAX_SHAPE_POINTS];
int selected_point_index = -1;
int move_point_index = -1;

// 0 - none,
// 1 - 1.00
// 2 - 0.10
// 3 - 0.01
const uint8_t kSimplifyMax = 4;
uint8_t simplify_mode_ = 0;
shape_point simplified_shape[MAX_SHAPE_POINTS];

double area_ = 0.0;
grid_point shape_center;
int final_shape_size = 0;
grid_point final_shape[MAX_SHAPE_POINTS];

void render();
void idle();
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void keyboard(unsigned char key, int x, int y);
void add_point_to_current_shape();
void update_center();
void move_shape_to_center();
void preview_simplified_shape();
void simplify_shape();
void write_shape();
void read_shape();

#define MAX_TEXT_BUFFER 256
char text_buffer[MAX_TEXT_BUFFER];
void text_print(int x, int y, const char* format, ...) {
    memset(text_buffer, 0, MAX_TEXT_BUFFER);
    va_list args;
    va_start(args, format);
    vsnprintf(text_buffer, MAX_TEXT_BUFFER, format, args);
    va_end(args);

    void* font = GLUT_BITMAP_9_BY_15;

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
    glOrtho(
        -grid_scale_factors_[grid_scale_index_] * GRID_SIZE,
        +grid_scale_factors_[grid_scale_index_] * GRID_SIZE,
        -grid_scale_factors_[grid_scale_index_] * GRID_SIZE,
        +grid_scale_factors_[grid_scale_index_] * GRID_SIZE,
        -1.0, 1.0);
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
    render_panel_frame(10, 10, 120, 50);

    glColor3f(1.0, 1.0, 1.0);
    text_print(20, 30, "X: %+10.4f", cursor_on_grid.x);
    text_print(20, 50, "Y: %+10.4f", cursor_on_grid.y);
}

void render_debug_panel() {

    if (debug_enable_ == 0) return;

    // Background color
    glColor4f(0.0, 0.0, 1.0, 0.5);
    glPushAttrib(GL_COLOR_BUFFER_BIT);
    render_panel_frame(SCREEN_SIZE - 110, 10, 400, 100);

    glColor3f(1.0, 1.0, 1.0);
    if (selected_point_index != -1) {
        text_print(20, SCREEN_SIZE - 90, "Selected: %d", selected_point_index);
    } else {
        text_print(20, SCREEN_SIZE - 90, "Selected: none");
    }

    text_print(20, SCREEN_SIZE - 70, "Area    : %10.4f", area_);
    text_print(20, SCREEN_SIZE - 50, "Center  : %+10.4f %+10.4f", shape_center.x, shape_center.y);
}

void render_shape() {

    glLineWidth(3.0);

    for (int k=0; k<MAX_SHAPES; ++k) {
        if (k == shape_index_) continue;

        glColor3f(0.5, 0.5, 0.5);
        glBegin(GL_LINE_LOOP);
        for (int i=0; i<MAX_SHAPE_POINTS; ++i) {
            if (shape[k][i].valid) {
                glVertex2d(shape[k][i].point.x, shape[k][i].point.y);
            }
        }
        glEnd();
    }

    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_LINE_LOOP);
    for (int i=0; i<MAX_SHAPE_POINTS; ++i) {
        if (shape[shape_index_][i].valid) {
            glVertex2d(shape[shape_index_][i].point.x, shape[shape_index_][i].point.y);
        }
    }
    glEnd();

    glPointSize(10.0);
    glBegin(GL_POINTS);
    for (int i=0; i<MAX_SHAPE_POINTS; ++i) {
        if (shape[shape_index_][i].valid) {

            if (selected_point_index == i) {
                glColor3f(1.0, 0.0, 1.0);
            }
            else {
                glColor3f(0.0, 0.5, 1.0);
            }

            glVertex2d(shape[shape_index_][i].point.x, shape[shape_index_][i].point.y);
        }
    }
    glEnd();
}

void render_simplified_shape() {

    if (simplify_mode_ == 0) return;

    glLineWidth(1.0);
    glColor3f(0.0, 1.0, 0.0);
    glBegin(GL_LINE_LOOP);
    for (int i=0; i<MAX_SHAPE_POINTS; ++i) {
        if (simplified_shape[i].valid) {
            glVertex2d(simplified_shape[i].point.x, simplified_shape[i].point.y);
        }
    }
    glEnd();
}

void render_shape_center() {

    if (final_shape_size < 3) return;

    glLineWidth(1.0);
    glColor3f(0.5, 0.5, 1.0);
    glBegin(GL_LINES);
        glVertex2d( shape_center.x - 1.0, shape_center.y);
        glVertex2d( shape_center.x + 1.0, shape_center.y);
        glVertex2d( shape_center.x, shape_center.y - 1.0);
        glVertex2d( shape_center.x, shape_center.y + 1.0);
    glEnd();
}

void render() {

    grid_mode();

    render_axes();
    render_grid();
    render_shape();
    render_simplified_shape();
    render_shape_center();

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
                shape[shape_index_][selected_point_index].valid = false;

                update_center();
            }
        }
    }
}

void calculate_cursor_on_grid() {

    cursor_on_grid.x = 2.0 * GRID_SIZE * cursor_on_screen.x / SCREEN_SIZE - GRID_SIZE;
    cursor_on_grid.y = -2.0 * GRID_SIZE * cursor_on_screen.y / SCREEN_SIZE + GRID_SIZE;

    cursor_on_grid.x *= grid_scale_factors_[grid_scale_index_];
    cursor_on_grid.y *= grid_scale_factors_[grid_scale_index_];
}

void find_selected_point() {

    selected_point_index = -1;
    for (int i=0; i<MAX_SHAPE_POINTS; ++i) {
        if (shape[shape_index_][i].valid == false) continue;
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
        shape[shape_index_][move_point_index].point = cursor_on_grid;

        update_center();
    }
}

void add_point_to_current_shape() {

    shape[shape_index_][shape_point_index].point = cursor_on_grid;
    shape[shape_index_][shape_point_index].valid = true;
    shape_point_index = (shape_point_index+1) % MAX_SHAPE_POINTS;

    update_center();
}

void keyboard(unsigned char key, int x, int y) {

	switch(key) {
        case 'd':
            debug_enable_ ^= 1; break;
        case 'o':
            // move shape center to origin.
            move_shape_to_center();
            break;
        case 's':
            simplify_mode_ = (simplify_mode_ + 1) % kSimplifyMax;
        case 'S':
            preview_simplified_shape();
            break;
        case 'a':
            simplify_shape();
            break;
        case 'w':
            write_shape();
            break;
        case 'r':
            read_shape();
            break;
        case 'z':
            grid_scale_index_ = (grid_scale_index_ + 1) % kMaxGridScaleIndex;
            break;
        case 'n':
            shape_index_ = (shape_index_ + 1) % MAX_SHAPES;
            update_center();
            break;
		case 27:
			exit(0);
			break;
	}
}

void idle() {
    // TODO
    usleep(20000);
}

void preview_simplified_shape() {

    if (simplify_mode_ == 0) return;

    double s_factor;
    switch (simplify_mode_) {
    case 1: s_factor = 1.0; break;
    case 2: s_factor = 10.0; break;
    case 3: s_factor = 100.0; break;
    default:
        return;
    }

    for (int i=0; i<MAX_SHAPE_POINTS; ++i) {
        simplified_shape[i].valid = shape[shape_index_][i].valid;
        simplified_shape[i].point.x = round(shape[shape_index_][i].point.x * s_factor) / s_factor;
        simplified_shape[i].point.y = round(shape[shape_index_][i].point.y * s_factor) / s_factor;
    }
}

void simplify_shape() {

    simplify_mode_ = 0;

    for (int i=0; i<MAX_SHAPE_POINTS; ++i) {
        shape[shape_index_][i] = simplified_shape[i];
    }

    update_center();
}

void update_center() {

    int final_shape_index = 0;
    for (int i=0; i<MAX_SHAPE_POINTS; ++i) {
        if (shape[shape_index_][i].valid) {
            final_shape[final_shape_index] = shape[shape_index_][i].point;
            final_shape_index++;
        }
    }
    final_shape_size = final_shape_index;

    area_ = 0.0;
    for (int i=0; i<final_shape_size; ++i) {
        int j = (i+1) % final_shape_size;
        area_ += 0.5 * (final_shape[i].x * final_shape[j].y - final_shape[j].x * final_shape[i].y);
    }

    shape_center.x = 0.0;
    shape_center.y = 0.0;
    for (int i=0; i<final_shape_size; ++i) {
        int j = (i+1) % final_shape_size;
        double common = (final_shape[i].x * final_shape[j].y - final_shape[j].x * final_shape[i].y);
        shape_center.x += (final_shape[i].x + final_shape[j].x) * common;
        shape_center.y += (final_shape[i].y + final_shape[j].y) * common;
    }
    shape_center.x /= 6.0 * area_;
    shape_center.y /= 6.0 * area_;
}

void move_shape_to_center() {

    for (int i=0; i<MAX_SHAPE_POINTS; ++i) {
        shape[shape_index_][i].point.x -= shape_center.x;
        shape[shape_index_][i].point.y -= shape_center.y;
    }

    update_center();
}

void write_shape() {

    // Print shape
    puts("");
    bool once = true;
    for(int i=0; i<MAX_SHAPE_POINTS; ++i) {
        if (shape[shape_index_][i].valid) {
            if (once) {
                once = false;
                printf("{");
            } else {
                puts(",");
            }
            printf("{%.3f, %.3f}", shape[shape_index_][i].point.x, shape[shape_index_][i].point.y);
        }
    }
    puts("}");

    // Save shape to design.poly
    char filename[32];
    sprintf(filename, "design-%02d.poly", shape_index_);
    FILE *fSave = fopen(filename, "wb");
    if (fSave != 0) {
        fwrite(shape[shape_index_], sizeof(shape_point), MAX_SHAPE_POINTS, fSave);
        fclose(fSave);
    }
}

void read_shape() {

    char filename[32];
    sprintf(filename, "design-%02d.poly", shape_index_);
    FILE *fLoad = fopen(filename, "rb");
    if (fLoad != 0) {
        fread(shape[shape_index_], sizeof(shape_point), MAX_SHAPE_POINTS, fLoad);
        fclose(fLoad);
        update_center();
    }
}

