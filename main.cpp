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

uint8_t move_and_rotate_mode_ = 0;
bool move_shape_enable_ = false;
bool rotate_shape_enable_ = false;
uint8_t edit_mode_ = 0;
const int kDashMax = 8;
int dash_index_ = 0;
uint16_t dash_patterns_[kDashMax] = {
    0xf0f0, 0x7878, 0x3c3c, 0x1e1e, 0x0f0f, 0x8787, 0xc3c3, 0xe1e1
};

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

int copy_shape_index_ = -1;
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

double area_[MAX_SHAPES] = {0};
grid_point shape_center[MAX_SHAPES];
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
void quit_application();
void load_shapes();
void flip_x_values();
void flip_y_values();
void paste_copied_shape(bool at_target);
void move_shape_with_mouse();
void rotate_shape_with_mouse();
void rotate_shape_by(double angle);
void shift_first_vertice();

double start_angle_ = 0.0;
double rotate_angle_ = 0.0;
void rotate_shape_start_angle();

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

    load_shapes();

	glutMainLoop();

	return 0;
}

void render_axes() {
    glLineWidth(2.0);
    if (edit_mode_ != 0) {
        glColor3f(1.0, 0.0, 0.0);
    } else {
        glColor3f(0.0, 0.0, 1.0);
    }
    glBegin(GL_LINES);
        glVertex2d(-GRID_SIZE, 0.0);
        glVertex2d( GRID_SIZE, 0.0);
        glVertex2d( 0.0, -GRID_SIZE);
        glVertex2d( 0.0,  GRID_SIZE);
    glEnd();
}

void render_grid() {

    glLineWidth(1.0);
    if (edit_mode_ != 0) {
        glColor4f(1.0, 0.0, 0.0, 0.4);
    } else {
        glColor4f(0.0, 0.0, 1.0, 0.4);
    }
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

void render_shape_index() {

    // Background color
    glColor4f(0.0, 1.0, 1.0, 0.3);
    glPushAttrib(GL_COLOR_BUFFER_BIT);
    render_panel_frame(SCREEN_SIZE - 50, SCREEN_SIZE - 110, 100, 40);

    glColor3f(1.0, 1.0, 1.0);
    text_print(SCREEN_SIZE - 100, SCREEN_SIZE - 25, "Shape #%2d", shape_index_);
}

void render_cursor_position() {

    // Background color
    glColor4f(0.0, 0.0, 0.0, 0.8);
    glPushAttrib(GL_COLOR_BUFFER_BIT);
    render_panel_frame(10, 100, 140, 50);

    glColor3f(1.0, 1.0, 1.0);
    text_print(110, 30, "X: %+10.4f", cursor_on_grid.x);
    text_print(110, 50, "Y: %+10.4f", cursor_on_grid.y);
}

void render_operation_mode() {
    // Background color
    if (edit_mode_ != 0) {
        glColor4f(0.8, 0.0, 0.0, 0.8);
    } else {
        glColor4f(0.0, 0.0, 0.8, 0.8);
    }
    glPushAttrib(GL_COLOR_BUFFER_BIT);
    render_panel_frame(10, 10, 80, 50);

    glColor3f(1.0, 1.0, 1.0);
    if (edit_mode_ != 0) {
        text_print(30, 40, "EDIT");
    } else {
        text_print(30, 40, "VIEW");
    }
}

void render_vertice_position() {

    if (selected_point_index == -1) return;

    glColor4f(0.0, 0.8, 0.0, 0.6);
    glPushAttrib(GL_COLOR_BUFFER_BIT);
    render_panel_frame(10, (SCREEN_SIZE>>1) - 90, 180, 40);

    grid_point* sp = &shape[shape_index_][selected_point_index].point;
    glColor3f(0.8, 1.0, 0.8);
    text_print((SCREEN_SIZE>>1) - 90, 30, "%+8.4f, %+8.4f", sp->x, sp->y);
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

    text_print(20, SCREEN_SIZE - 70, "Area    : %10.4f", area_[shape_index_]);
    text_print(20, SCREEN_SIZE - 50, "Center  : %+10.4f %+10.4f", shape_center[shape_index_].x, shape_center[shape_index_].y);
    text_print(20, SCREEN_SIZE - 30, "Rotation: %6.1f", rotate_angle_ * 180.0 / M_PI);
}

void render_shape() {

    glLineWidth(3.0);

    for (int k=0; k<MAX_SHAPES; ++k) {
        if (k == shape_index_) continue;

        if (copy_shape_index_ == k) {
            glEnable(GL_LINE_STIPPLE);
            glLineStipple(1, dash_patterns_[dash_index_]);
        }
        else {
            glDisable(GL_LINE_STIPPLE);
        }

        glColor3f(0.5, 0.5, 0.5);
        glBegin(GL_LINE_LOOP);
        for (int i=0; i<MAX_SHAPE_POINTS; ++i) {
            if (shape[k][i].valid) {
                glVertex2d(shape[k][i].point.x, shape[k][i].point.y);
            }
        }
        glEnd();

        if (copy_shape_index_ == k) {
            glDisable(GL_LINE_STIPPLE);
        }
    }

    if (copy_shape_index_ == shape_index_) {
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, dash_patterns_[dash_index_]);
    }
    else {
        glDisable(GL_LINE_STIPPLE);
    }

    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_LINE_LOOP);
    for (int i=0; i<MAX_SHAPE_POINTS; ++i) {
        if (shape[shape_index_][i].valid) {
            glVertex2d(shape[shape_index_][i].point.x, shape[shape_index_][i].point.y);
        }
    }
    glEnd();

    if (copy_shape_index_ == shape_index_) {
         glDisable(GL_LINE_STIPPLE);
    }

    if (edit_mode_ == 0) return;

    bool first_vertice = false;
    for (int i=0; i<MAX_SHAPE_POINTS; ++i) {
        if (shape[shape_index_][i].valid) {
            if (!first_vertice) {
                first_vertice = true;
                glPointSize(10.0);
                glColor3f(0.0, 1.0, 1.0);
            } else {
                glPointSize(10.0);
                glColor3f(0.0, 0.5, 1.0);
            }
            if (selected_point_index == i) {
                glPointSize(15.0);
                glColor3f(1.0, 0.0, 1.0);
            }

            glBegin(GL_POINTS);
            glVertex2d(shape[shape_index_][i].point.x, shape[shape_index_][i].point.y);
            glEnd();
        }
    }
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

    const double c_size = 0.25;
    glLineWidth(1.0);
    glColor3f(0.5, 0.5, 1.0);
    glBegin(GL_LINES);
        glVertex2d( shape_center[shape_index_].x - c_size, shape_center[shape_index_].y);
        glVertex2d( shape_center[shape_index_].x + c_size, shape_center[shape_index_].y);
        glVertex2d( shape_center[shape_index_].x, shape_center[shape_index_].y - c_size);
        glVertex2d( shape_center[shape_index_].x, shape_center[shape_index_].y + c_size);
    glEnd();
}

void render_rotation_guide() {

    if (move_and_rotate_mode_ != 0 && rotate_shape_enable_) {
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, dash_patterns_[dash_index_]);
        glLineWidth(1.0);
        glColor3f(0.0, 1.0, 0.0);
        glBegin(GL_LINES);
        glVertex2d(cursor_on_grid.x, cursor_on_grid.y);
        glVertex2d(shape_center[shape_index_].x, shape_center[shape_index_].y);
        glEnd();
        glDisable(GL_LINE_STIPPLE);
    }
}

void render_vertice_order() {

    grid_point* c = &shape_center[shape_index_];

    double a = 0.0;
    double da = 0.0;
    glLineWidth(2.0);
    glColor3f(0.0, 0.7, 0.0);
    glBegin(GL_LINE_STRIP);
    for (a=0.25*M_PI; a<=0.75*M_PI; a+=0.1) {
        glVertex2d(c->x + 0.2 * cos(a), c->y + 0.2 * sin(a));
    }
    glEnd();
    if (area_[shape_index_] < 0.0) {
        a = 0.25*M_PI;
        da = 25.0 * M_PI / 180.0;
    } else {
        a = 0.75*M_PI;
        da = -25.0 * M_PI / 180.0;
    }

    glBegin(GL_LINE_STRIP);
        glVertex2d(c->x + 0.1 * cos(a+da), c->y + 0.1 * sin(a+da));
        glVertex2d(c->x + 0.2 * cos(a), c->y + 0.2 * sin(a));
        glVertex2d(c->x + 0.3 * cos(a+da), c->y + 0.3 * sin(a+da));
    glEnd();
}

void render() {

    grid_mode();

    render_axes();
    render_grid();
    render_shape();
    render_simplified_shape();
    render_shape_center();
    render_rotation_guide();
    render_vertice_order();

    ui_mode();

    render_cursor_position();
    render_debug_panel();
    render_vertice_position();
    render_operation_mode();
    render_shape_index();
}

void mouse(int button, int state, int x, int y) {

    if (edit_mode_ == 0) return;

    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            if (move_and_rotate_mode_) {
                move_shape_enable_ = true;
                rotate_shape_enable_ = false;
            }
            else {
                if (selected_point_index == -1) {
                    // Add new point, if no point is selected.
                    add_point_to_current_shape();
                }
                else {
                    // Move point, if one is selected.
                    move_point_index = selected_point_index;
                }
            }
        }
        else if (state == GLUT_UP) {
            if (move_and_rotate_mode_) {
                move_shape_enable_ = false;
                rotate_shape_enable_ = false;
                move_and_rotate_mode_ = 0;
            }
            else {
                move_point_index = -1;
            }
        }
    }
    else if (button == GLUT_RIGHT_BUTTON) {
        if (state == GLUT_DOWN) {
            if (move_and_rotate_mode_) {
                rotate_shape_start_angle();
                rotate_shape_enable_ = true;
                move_shape_enable_ = false;
            }
            else {
                // Delete (disable) selected point, if any.
                if (selected_point_index != -1) {
                    shape[shape_index_][selected_point_index].valid = false;
                    update_center();
                }
            }
        }
        else if (state == GLUT_UP) {
            if (move_and_rotate_mode_) {
                rotate_shape_enable_ = false;
                move_shape_enable_ = false;
                move_and_rotate_mode_ = 0;
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
        double r2 = shape[shape_index_][i].point.distance_square(cursor_on_grid);
        double sc2 = grid_scale_factors_[grid_scale_index_];
        sc2 *= sc2;
        if (r2 <= SELECT_DISTANCE_SQ * sc2) {
            selected_point_index = i;
            break;
        }
    }
}

void motion(int x, int y) {

    if (edit_mode_ == 0) return;

    cursor_on_screen.x = x;
    cursor_on_screen.y = y;

    calculate_cursor_on_grid();

    if (move_and_rotate_mode_ != 0) {
        if (move_shape_enable_) {
            move_shape_with_mouse();
        } else if (rotate_shape_enable_) {
            rotate_shape_with_mouse();
        }
    } else {

        if (move_point_index != -1) {
            shape[shape_index_][move_point_index].point = cursor_on_grid;

            update_center();
        }
        else {

            find_selected_point();
        }
    }
}

void add_point_to_current_shape() {

    shape[shape_index_][shape_point_index].point = cursor_on_grid;
    shape[shape_index_][shape_point_index].valid = true;
    shape_point_index = (shape_point_index+1) % MAX_SHAPE_POINTS;

    update_center();
}

void process_view_keys(unsigned char key) {
    switch(key) {
        case 'd':
            debug_enable_ ^= 1; break;
        case 'z':
            grid_scale_index_ = (grid_scale_index_ + 1) % kMaxGridScaleIndex;
            break;
        case 'e':
            edit_mode_ ^= 1; break;
        case 27:
            quit_application(); break;
    }
}

void process_edit_keys(unsigned char key) {

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
        case 'c':
            copy_shape_index_ = shape_index_; break;
        case 'p':
            paste_copied_shape(true); break;
        case 'P':
            paste_copied_shape(false); break;
        case 'w':
            write_shape();
            break;
        case 'r':
            read_shape();
            break;
        case 't':
            rotate_shape_by(-90.0);
            break;
        case 'T':
            rotate_shape_by(90.0);
            break;
        case 'm':
            move_and_rotate_mode_ ^= 1;
            break;
        case 'h':
            flip_x_values(); break;
        case 'v':
            flip_y_values(); break;
        case 'z':
            grid_scale_index_ = (grid_scale_index_ + 1) % kMaxGridScaleIndex;
            break;
        case 'n':
            shape_index_ = (shape_index_ + 1) % MAX_SHAPES;
            update_center();
            break;
        case 'f':
            shift_first_vertice(); break;
        case 'e':
            edit_mode_ ^= 1; break;
		case 27:
            quit_application();
			break;
	}
}

void keyboard(unsigned char key, int x, int y) {

    if (edit_mode_ != 0) {
        process_edit_keys(key);
    } else {
        process_view_keys(key);
    }

}

void idle() {
    dash_index_ = (dash_index_ + 1) % kDashMax;
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

    if (simplify_mode_ != 0) {

        simplify_mode_ = 0;

        for (int i=0; i<MAX_SHAPE_POINTS; ++i) {
            shape[shape_index_][i] = simplified_shape[i];
        }

        update_center();
    }
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

    area_[shape_index_] = 0.0;
    for (int i=0; i<final_shape_size; ++i) {
        int j = (i+1) % final_shape_size;
        area_[shape_index_] += 0.5 * (final_shape[i].x * final_shape[j].y - final_shape[j].x * final_shape[i].y);
    }

    shape_center[shape_index_].x = 0.0;
    shape_center[shape_index_].y = 0.0;
    for (int i=0; i<final_shape_size; ++i) {
        int j = (i+1) % final_shape_size;
        double common = (final_shape[i].x * final_shape[j].y - final_shape[j].x * final_shape[i].y);
        shape_center[shape_index_].x += (final_shape[i].x + final_shape[j].x) * common;
        shape_center[shape_index_].y += (final_shape[i].y + final_shape[j].y) * common;
    }
    shape_center[shape_index_].x /= 6.0 * area_[shape_index_];
    shape_center[shape_index_].y /= 6.0 * area_[shape_index_];
}

void move_shape_to_center() {

    for (int i=0; i<MAX_SHAPE_POINTS; ++i) {
        shape[shape_index_][i].point.x -= shape_center[shape_index_].x;
        shape[shape_index_][i].point.y -= shape_center[shape_index_].y;
    }

    update_center();
}

void write_shape() {

    // Print shape
    bool once = true;
    for(int i=0; i<MAX_SHAPE_POINTS; ++i) {
        if (shape[shape_index_][i].valid) {
            if (once) {
                once = false;
                printf("\n{");
            } else {
                puts(",");
            }
            printf("{%.3f, %.3f}", shape[shape_index_][i].point.x, shape[shape_index_][i].point.y);
        }
    }
    if (once == false) puts("}");

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

void quit_application() {

    for (int i=0; i<MAX_SHAPES; ++i) {
        shape_index_ = i;
        write_shape();
    }

    exit(0);
}

void load_shapes() {
    for (int i=0; i<MAX_SHAPES; ++i) {
        shape_index_ = i;
        read_shape();
    }
}

void flip_x_values() {
    grid_point* p;
    for (int i=0; i<MAX_SHAPES; ++i) {
        p = &shape[shape_index_][i].point;
        p->x = 2.0 * shape_center[shape_index_].x - p->x;
    }
}

void flip_y_values() {
    grid_point* p;
    for (int i=0; i<MAX_SHAPES; ++i) {
        p = &shape[shape_index_][i].point;
        p->y = 2.0 * shape_center[shape_index_].y - p->y;
    }

}

void paste_copied_shape(bool at_target) {

    if (copy_shape_index_ == -1 || copy_shape_index_ == shape_index_) return;

    double ox, oy;
    if (at_target) {
        ox = shape_center[shape_index_].x - shape_center[copy_shape_index_].x;
        oy = shape_center[shape_index_].y - shape_center[copy_shape_index_].y;
    } else {
        ox = oy = 0.0;
    }

    for (int i=0; i<MAX_SHAPE_POINTS; ++i) {
        shape[shape_index_][i].point.x = shape[copy_shape_index_][i].point.x + ox;
        shape[shape_index_][i].point.y = shape[copy_shape_index_][i].point.y + oy;
        shape[shape_index_][i].valid = shape[copy_shape_index_][i].valid;
    }

    copy_shape_index_ = -1;
}

void move_shape_with_mouse() {

    grid_point pC = shape_center[shape_index_];
    grid_point *p;
    for (int i=0; i<MAX_SHAPE_POINTS; ++i) {
        p = &shape[shape_index_][i].point;
        p->x = (p->x - pC.x) + cursor_on_grid.x;
        p->y = (p->y - pC.y) + cursor_on_grid.y;
    }
    update_center();
}

grid_point rotate_point(grid_point& r1, double angle) {

    grid_point r2;

    r2.x = r1.x * cos(angle) - r1.y * sin(angle);
    r2.y = r1.x * sin(angle) + r1.y * cos(angle);

    return r2;
}

void rotate_shape_with_mouse() {
    double dy = cursor_on_grid.y - shape_center[shape_index_].y;
    double dx = cursor_on_grid.x - shape_center[shape_index_].x;
    rotate_angle_ = atan2(dy, dx);
    double delta_angle = rotate_angle_ - start_angle_;

    grid_point c = shape_center[shape_index_];
    grid_point *p;
    grid_point r1, r2;
    for (int i=0; i<MAX_SHAPE_POINTS; ++i) {
        p = &shape[shape_index_][i].point;
        r1.x = p->x - c.x;
        r1.y = p->y - c.y;
        r2 = rotate_point(r1, delta_angle);
        p->x = c.x + r2.x;
        p->y = c.y + r2.y;
    }

    start_angle_ = rotate_angle_;
}

void rotate_shape_start_angle() {

    double dy = cursor_on_grid.y - shape_center[shape_index_].y;
    double dx = cursor_on_grid.x - shape_center[shape_index_].x;
    start_angle_ = atan2(dy, dx);
    rotate_angle_ = 0.0;
}

void rotate_shape_by(double angle) {
    double r_angle = angle * M_PI / 180.0;
    double dy = cursor_on_grid.y - shape_center[shape_index_].y;
    double dx = cursor_on_grid.x - shape_center[shape_index_].x;

    grid_point c = shape_center[shape_index_];
    grid_point *p;
    grid_point r1, r2;
    for (int i=0; i<MAX_SHAPE_POINTS; ++i) {
        p = &shape[shape_index_][i].point;
        r1.x = p->x - c.x;
        r1.y = p->y - c.y;
        r2 = rotate_point(r1, r_angle);
        p->x = c.x + r2.x;
        p->y = c.y + r2.y;
    }
}

void shift_first_vertice() {

    // TODO
}

