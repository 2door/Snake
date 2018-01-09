#ifdef __APPLE__
#include <GLUT/glut.h> 
#else

#include <GL/glew.h>
#include <GL/glut.h> 
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string>
#include "grid.h"
#include "snake.h"
#include "pallet.h"
#include "buttonlist.h"
#include "load_and_bind_texture.h"

enum tex { HEADFRONT=0, HEADLEFT=1, HEADRIGHT=2, HEADBOT=3, HEADTOP=4,
		   INTER=5, SEGMENT=6, TURN=7, TAIL=8, TEXNUM=9 };
enum cube_sides { L=0, R=1, T=2, B=3, F=4, N=5 };

float arena_size = 600.0f;		// The size, in world units, of the play area
float screen_padding = 5.0f;	// In world coordinates/sizes
float hud_height = 50.0f;		// The height of the HUD in the world
float screen_height;	// The total height of the viewport and screen
float screen_width;		// The total width of the viewport and screen
float h_limit;			// The horizontal limit at which can draw
float v_limit;			// The vertical limit at which can draw
float grid_left;		// The left edge of the grid (x-coordinate)
float grid_right;		// The right edge of the grid (x-coordinate)
float grid_top;			// The top edge of the grid (y-coordinate)
float grid_bot;			// The bottom edge of the grid (y-coordinate)
float text_size = .2f;
float extend;	// Size by which a 3D snake segment should be elongated	

// Cube edges: Right, Left etc.
float c_r = 1.0f;
float c_l = .0f;
float c_t = .0f;
float c_b = -1.0f;
float c_f = .0f;
float c_n = -1.0f;

// Coordinates of cube corners divided into sides
static float cube[6][4][3] = {
	{{c_l, c_t, c_n}, {c_l, c_t, c_f}, {c_l, c_b, c_f}, {c_l, c_b, c_n}}, // Left
	{{c_r, c_b, c_n}, {c_r, c_b, c_f}, {c_r, c_t, c_f}, {c_r, c_t, c_n}}, // Right
	{{c_r, c_t, c_n}, {c_r, c_t, c_f}, {c_l, c_t, c_f}, {c_l, c_t, c_n}}, // Top
	{{c_l, c_b, c_n}, {c_l, c_b, c_f}, {c_r, c_b, c_f}, {c_r, c_b, c_n}}, // Bottom
	{{c_l, c_b, c_f}, {c_l, c_t, c_f}, {c_r, c_t, c_f}, {c_r, c_b, c_f}}, // Far
	{{c_l, c_t, c_n}, {c_r, c_t, c_n}, {c_r, c_b, c_n}, {c_l, c_b, c_n}}  // Near
};
static int tex_source_coords[4][2] {{0, 0}, {900, 0}, {900, 900}, {0, 900}};

bool menu = true;		// If game is in menu
bool running = false;	// If game is running at the moment
bool game_over = false;	// If the game has been lost
bool moved = false;		// If the snake has moved since the last direction change
bool loop = true;		// If the snake is allowed to loop at edges of screen
bool display_grid = false;
int ticks;			// Ticks that have been counted. Resets depending on difficulty
int menu_screen;	// The current menu screen (check Destination in button.h for options)
int y_tilt = 0;
int x_tilt = 0;
unsigned int grid_size = 15;	// The order of the grid/matrix. Must be >5
unsigned int difficulty = 0;		// The current difficulty level
unsigned int difficulty_step = 2;	// The required score change for difficulty increase
unsigned int max_difficulty = 9;
unsigned int delay_step = 10;		// Tick difference between difficulties
unsigned int max_delay = 140;		// The largest delay (in ticks) between snake steps
unsigned int g_bitmap_text_handle = 0;
unsigned int textures[TEXNUM];
Grid* grid;				// Stores grid cell coordinates
Snake* snake;			// Stores snake information and allows snake movement
Pallet* pallet;			// Stores food pallet info and provides pallet functionality
ButtonList* buttonList;	// Stores GUI buttons and their information/effects

unsigned int make_bitmap_text() {
	unsigned int handle_base = glGenLists(256); 
	for (int i = 0; i < 256; i++) {
		// a new list for each character
		glNewList(handle_base+i, GL_COMPILE);
			glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_10, i);
		glEndList();
	}
	return handle_base;
}

void load_and_bind_textures()
{
	textures[HEADFRONT] = load_and_bind_texture("./images/headfront.png");
	textures[HEADLEFT] = load_and_bind_texture("./images/headleft.png");
	textures[HEADRIGHT] = load_and_bind_texture("./images/headright.png");
	textures[HEADBOT] = load_and_bind_texture("./images/headbot.png");
	textures[HEADTOP] = load_and_bind_texture("./images/headtop.png");
	textures[INTER] = load_and_bind_texture("./images/inter.png");
	textures[SEGMENT] = load_and_bind_texture("./images/segment.png");
	textures[TURN] = load_and_bind_texture("./images/turn.png");
	textures[TAIL] = load_and_bind_texture("./images/tail.png");
}

void draw_text(const char* s) {
	int len = strlen(s);
	for (int i = 0; i < len; i++) {
		glutStrokeCharacter(GLUT_STROKE_ROMAN, s[i]);
	}
}
void draw_text(std::string s) {
	int len = s.length();
	for (int i = 0; i < len; i++) {
		glutStrokeCharacter(GLUT_STROKE_ROMAN, s[i]);
	}
}

float str_width(const char* s) {
	int len = strlen(s);
	int total_width = 0;
	for(int i = 0; i < len; i++) {	
		total_width += glutStrokeWidth(GLUT_STROKE_ROMAN, s[i]) ;
	}
	return total_width;
}
float str_width(std::string s) {
	int len = s.length();
	int total_width = 0;
	for(int i = 0; i < len; i++) {	
		total_width += glutStrokeWidth(GLUT_STROKE_ROMAN, s[i]) ;
	}
	return total_width;
}

int normalize(int x, int a1, int a2, int b1, int b2) {
	return b1 + ( (x - a1) * (b2 - b1) / (a2 - a1) );
}

/*
* Method that draws a square of side size 1.0
* scaling on x or y by a number N will result in a respective side of size N
*/
void draw_square() {
	static float vertex[4][2] = {
		{.0f, .0f},
		{1.0f, .0f},
		{1.0f, -1.0f},
		{.0f, -1.0f}
	};

	glBegin(GL_LINE_LOOP);
		for(int i = 0; i < 4; i++) {
			glVertex2fv(vertex[i]);
		}
	glEnd();
}

void draw_cube() {
	glColor3f(1.0f, 1.0f, 1.0f);
	for(size_t i = 0; i < 6; i++) {
		glBegin(GL_QUADS);
			for(size_t j = 0; j < 4; j++) {
				glVertex3fv(cube[i][j]);
			}
		glEnd();
	}
}

/*
* Method that draws a grid using the coordinates stored in the Grid structure
*/
void draw_grid() {
	for(int i = 0; i < grid_size; i++) {
			for(int j = 0; j < grid_size; j++) {
				glPushMatrix();
					Cell* new_cell = grid->GetCellAt(i, j);
					glTranslatef(new_cell->GetX(), new_cell->GetY(), .0f);
					glScalef(grid->GetCellSize(), grid->GetCellSize(), 1.0f);
					draw_square();
				glPopMatrix();
			}
		}
}

void draw_3D_segment(float x, float y, float z) {
	glPushMatrix();
		glTranslatef(x, y, z);
		glScalef(grid->GetCellSize(), grid->GetCellSize(), grid->GetCellSize());
		draw_cube();
	glPopMatrix();
}
void draw_head(unsigned int dir, float x, float y, float z) {
	glTranslatef(x, y, z);
	glScalef(grid->GetCellSize(), grid->GetCellSize(), grid->GetCellSize());
	cube_sides order[6] = {L, R, B, T, F, N};
	switch(dir) {
		case LEFT: order[0] = R; order[1] = L; break;
		case UP: order[0] = B; order[2] = T;
				 order[3] = L; order[4] = R; break;
		case DOWN: order[0] = T; order[2] = B;
				   order[3] = R; order[4] = L; break;
	}
	glBindTexture(GL_TEXTURE_2D, textures[INTER]);
	glBegin(GL_QUADS);
		for(size_t i = 0; i < 4; i++) {
			glTexCoord2f(tex_source_coords[i][0], tex_source_coords[i][1]);
			glVertex3fv(cube[L][i]);
		}
	glEnd();
	glBindTexture(GL_TEXTURE_2D, textures[HEADFRONT]);
	glBegin(GL_QUADS);
		for(size_t i = 0; i < 4; i++) {
			glTexCoord2f(tex_source_coords[i][0], tex_source_coords[i][1]);
			glVertex3fv(cube[R][i]);
		}
	glEnd();
	glBindTexture(GL_TEXTURE_2D, textures[HEADLEFT]);
	glBegin(GL_QUADS);
		for(size_t i = 0; i < 4; i++) {
			glTexCoord2f(tex_source_coords[i][0], tex_source_coords[i][1]);
			glVertex3fv(cube[T][i]);
		}
	glEnd();
	glBindTexture(GL_TEXTURE_2D, textures[HEADRIGHT]);
	glBegin(GL_QUADS);
		for(size_t i = 0; i < 4; i++) {
			glTexCoord2f(tex_source_coords[i][0], tex_source_coords[i][1]);
			glVertex3fv(cube[B][i]);
		}
	glEnd();
	glBindTexture(GL_TEXTURE_2D, textures[HEADBOT]);
	glBegin(GL_QUADS);
		for(size_t i = 0; i < 4; i++) {
			glTexCoord2f(tex_source_coords[i][0], tex_source_coords[i][1]);
			glVertex3fv(cube[F][i]);
		}
	glEnd();
	glBindTexture(GL_TEXTURE_2D, textures[HEADTOP]);
	glBegin(GL_QUADS);
	fflush(stdout);
		for(size_t i = 0; i < 4; i++) {
			glTexCoord2f(tex_source_coords[i][0], tex_source_coords[i][1]);
			glVertex3fv(cube[N][i]);
		}
	glEnd();
}

void draw_3D_snake() {
	unsigned int** positions = snake->GetSnakePosition();
	for(int i = 0; i < snake->GetLength(); i++) {
		glPushMatrix();
			Cell* new_cell = grid->GetCellAt(positions[i][0], positions[i][1]);
			if(i == 0) {
				draw_head(positions[i][2], new_cell->GetX(), new_cell->GetY(), .0f);
			// } else if(i == snake->GetLength() - 1) {
			// 	draw_tail(positions[i][2]);
			// } else {
			// 	if(positions[i-1][2] != positions[i+1][2]) {
			// 		draw_turn(positions[i-1][2], positions[i+1][2]);
			// 	} else {
			// 		draw_segment(positions[i][2]);
			// 	}
			// }
			} else draw_3D_segment(new_cell->GetX(), new_cell->GetY(), .0f);
		glPopMatrix();
	}

	for ( int i = 0; i < snake->GetLength(); i++) {
		delete [] positions[i];
	}
	delete [] positions;
}

void draw_pallet() {
	Cell* new_cell = grid->GetCellAt(pallet->GetY(), pallet->GetX());
	glPushMatrix();
		glTranslatef(new_cell->GetX(), new_cell->GetY(), .0f);
		glScalef(grid->GetCellSize() - 0.5f, grid->GetCellSize() - 0.5f, 1.0f);
		glRectf(.0f, .0f, 1.0f, -1.0f);
	glPopMatrix();
}

void draw_header(const char* text) {
	glPushMatrix();
		float h_center_offset = -str_width(text)/(2/text_size);
		float v_center_offset = v_limit - screen_padding - (hud_height/2);
		glTranslatef(.0f + h_center_offset, .0f + v_center_offset, .0f);
		glScalef(text_size, text_size, 1.0f);
		draw_text(text);
	glPopMatrix();
}

void draw_state() {
	const char* text;
	if(game_over) {
		text = "Game Over!";
	} else {
		text = "Eat the Pallets!";
	}
	draw_header(text);
}

void draw_score() {
	std::string text("Score: ");
	text.append(std::to_string(snake->GetScore()));

	glPushMatrix();
		float h_center_offset = -h_limit + screen_padding;
		float v_center_offset = v_limit - screen_padding - (hud_height/2);
		glTranslatef(.0f + h_center_offset, .0f + v_center_offset, .0f);
		glScalef(text_size, text_size, 1.0f);
		draw_text(text);
	glPopMatrix();
}

void check_head_collisions() {
	// If head collide with body
	if(snake->Bite() && running) {
		running = false;
		game_over = true;
		glutPostRedisplay();
	}
	// If head collide w pallet
	unsigned int** positions = snake->GetSnakePosition();
	if(positions[0][0] == pallet->GetY() &&
			positions[0][1] == pallet->GetX()) {
		int palletX;
		int palletY;
		do{
			palletX = (int) ( rand() % ( grid_size - 1 ));
			palletY = (int) ( rand() % ( grid_size - 1 ));
		} while(!pallet->Reposition(palletX, palletY));
		snake->EatPallet();
		if(difficulty < max_difficulty && 
				snake->GetScore() >= difficulty * difficulty_step) {
			difficulty++;
		}
	}
}

void quit_game() {
	grid->Delete();
	snake->Delete();
	buttonList->Delete();
	delete grid;
	delete snake;
	delete buttonList;
	exit(0);
}

void keyboard(unsigned char key, int, int) {
    switch(key) {
        case 'q': 	quit_game(); break;   // Press q to force exit application
		case 'p':	if(!menu) {
						running = !running;
					}
					break;
		case 'y': 	y_tilt--; break;
		case 'Y': 	y_tilt++; break;
		case 'x': 	x_tilt--; break;
		case 'X':	x_tilt++; break;
    }
    glutPostRedisplay();
}

void special_keys(int key, int x, int y) {
	switch(key) {
		case GLUT_KEY_UP:
			if(moved) {
				if(snake->SetDirection(UP)) {
					moved = false;
				}
			}
			break;
		case GLUT_KEY_RIGHT:
			if(moved) {
				if(snake->SetDirection(RIGHT)) {
					moved = false;
				}
			}
			break;
		case GLUT_KEY_DOWN:
			if(moved) {
				if(snake->SetDirection(DOWN)) {
					moved = false;
				}
			}	
			break;
		case GLUT_KEY_LEFT:
			if(moved) {
				if(snake->SetDirection(LEFT)) {
					moved = false;
				}
			}
			break;
	}
}

// void reshape(int w, int h)
// {
// 	glViewport(0, 0, w, h);
// 	glMatrixMode(GL_PROJECTION);
// 	glLoadIdentity();
    
//     //gluPerspective(40.0, 1.0f, 1.0, 5.0);

// 	glutPostRedisplay();
// }

void display_main_menu() {
	draw_header("Main Menu");
	buttonList->DrawButtons();
}

void display_options() {
	draw_header("Options");
	buttonList->DrawButtons();
}

void display_instructions() {
	draw_header("Instructions");
}

void display_gui() {
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glColor3f(1.0f, 1.0f, 1.0f);
	// Draw the separator between the HUD and play area
	glBegin(GL_LINES);
		glVertex2f(-h_limit, grid_top + screen_padding);
		glVertex2f(h_limit, grid_top + screen_padding);
	glEnd();

	glMatrixMode(GL_MODELVIEW);
	if(menu_screen == MAIN) {
		display_main_menu();
	} else if(menu_screen == OPTIONS) {
		display_options();
	} else if(menu_screen == INSTRUCTIONS) {
		display_instructions();
	} else if(menu_screen == GAME) {
		menu = false;
		running = true;
	} else if(menu_screen == QUIT) {
		quit_game();
	}
	glutSwapBuffers();
}

void display_game() {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(x_tilt, y_tilt, 2, // eye position
			  0, 0, 0, // reference point
			  0, 1, 0  // up vector
		);

		glColor3f(1.0f, 1.0f, 1.0f);
		// Draw the separator between the HUD and play area
		glBegin(GL_LINES);
			glVertex2f(-h_limit, grid_top + screen_padding);
			glVertex2f(h_limit, grid_top + screen_padding);
		glEnd();

		// Draw the grid on which the snake and pallets will be displayed
		if(display_grid) {
			draw_grid();
		} else {
			// Draw edge
		}
		glEnable(GL_TEXTURE_2D);
		// Draw the snake
		draw_3D_snake();
		// Draw the pallet
		glDisable(GL_TEXTURE_2D);
		glColor3f(.3f, .6f, .3f);
		draw_pallet();
		//draw HUD text
		glColor3f(1.0f, 1.0f, 1.0f);
		draw_state();
		draw_score();
    glutSwapBuffers();
}

void display() {
	if(menu) {
		display_gui();
	} else {
		display_game();
	}
}

void idle() {
	usleep(1000);	// Microsectonds. 1000 = 1 millisecond
	if(running) {
		ticks++;
		if( (ticks == max_delay - ( difficulty * delay_step ))) {
			if(snake->Move() != -1) {
				running = false;
				game_over = true;
			}
			ticks = 0;
			moved = true;
			glutPostRedisplay();
		}
		check_head_collisions();
	}
}

void mouse_action(int button, int state, int x, int y) {
	if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		int** bounds = buttonList->GetButtonBounds();
		x = normalize(x, 0, screen_width, -h_limit, h_limit);
		y = normalize(y, 0, screen_height, v_limit, -v_limit);
		int checks = buttonList->GetCount();

		for(int i = 0; i < checks; i++) {
			if(y < bounds[i][0] && y > bounds[i][1] &&
			   		x > bounds[i][2] && x < bounds[i][3]) {
				menu_screen = bounds[i][4];
				switch(menu_screen) {
					case MAIN:
						buttonList->Refresh();
						buttonList->AddButton("Play", GAME);
						buttonList->AddButton("Options", OPTIONS);
						buttonList->AddButton("Instructions", INSTRUCTIONS);
						buttonList->AddButton("Quit", QUIT);
						break;
					case GAME:
						buttonList->Refresh();
						menu = false;
						running = true;
						break;
					case GRID:
						display_grid = !display_grid;
						menu_screen = OPTIONS;
						goto options;
						break;
					case LOOP:
						loop = !loop;
						snake->SetLoop(loop);
						menu_screen = OPTIONS;
						goto options;
						break;
					case OPTIONS:
						options:
						buttonList->Refresh();
						buttonList->AddButton("Back", MAIN);
						if(loop) {
							buttonList->AddButton("Loop: ON", LOOP);
						} else {
							buttonList->AddButton("Loop: OFF", LOOP);
						}
						if(display_grid) {
							buttonList->AddButton("Grid: ON", GRID);
						} else {
							buttonList->AddButton("Grid: OFF", GRID);							
						}
						break;
					case INSTRUCTIONS:
						break;
					case QUIT:
						quit_game();
						break;
					case PAUSE:
						break;
				}
			}
		}
		for ( int i = 0; i < checks; i++) {
			delete [] bounds[i];
		}
		delete [] bounds;
		glutPostRedisplay();
	}
}

void init_structs() {
	screen_height = arena_size + screen_padding + hud_height;
	screen_width = arena_size + screen_padding;
	h_limit = screen_width / 2;
	v_limit = screen_height / 2;
	grid_left = -h_limit + screen_padding;
	grid_right = h_limit - screen_padding;
	grid_top = v_limit - hud_height - screen_padding;
	grid_bot = -v_limit + screen_padding;
	grid = new Grid(arena_size, grid_size, screen_padding,
					grid_left,
					grid_top);
	extend = grid->GetCellSize() / 2;
	snake = new Snake(3, 2, grid_size, loop);

	srand(time(NULL));
	int palletX = (int) ( rand() % ( grid_size - 1 ));
	int palletY = (int) ( rand() % ( grid_size - 1 ));
	pallet = new Pallet(palletX, palletY);
	ticks = 0;

	menu_screen = 0;
	int v_center_offset = v_limit - hud_height - screen_padding;
	buttonList = new ButtonList(0 + v_center_offset, screen_width, 40);
	buttonList->AddButton("Play", GAME);
	buttonList->AddButton("Options", OPTIONS);
	buttonList->AddButton("Instructions", INSTRUCTIONS);
	buttonList->AddButton("Quit", QUIT);
}

void init_gl(int argc, char* argv[]) {
	load_and_bind_textures();

    // Set viewport size (=scren size) and orthographic viewing
	glViewport(0, 0, screen_width, screen_height);
	glMatrixMode(GL_PROJECTION); 
	glLoadIdentity();
	// Specify a projection with this view volume, centred on origin 
	// Takes LEFT, RIGHT, BOTTOM, TOP, NEAR and FAR
	glOrtho(-h_limit, h_limit, -v_limit, v_limit, -10000, 10000);
	glEnable(GL_DEPTH_TEST);
	glClearColor(.0f, 0.2f, .0f, 1.0f);
	g_bitmap_text_handle = make_bitmap_text();
}

int main(int argc, char* argv[]) {
	// For a properly working game, grid order must be greater than 5
	if(grid_size > 5 && arena_size > 200) {
		init_structs();
		glutInit(&argc, argv);
		glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH);
		glutInitWindowSize(screen_width, screen_height);
		glutInitWindowPosition(0, 0);
		glutCreateWindow("Snake");

	#ifndef __APPLE__
		GLenum err = glewInit();
		if(GLEW_OK != err) {
			fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
			exit(1);
		}
		fprintf(stderr, "Using GLEW %s\n", glewGetString(GLEW_VERSION));
	#endif

		glutKeyboardFunc(keyboard); 
		glutSpecialFunc(special_keys);
		glutMouseFunc(mouse_action);
		// glutReshapeFunc(reshape); 
		glutDisplayFunc(display);
		glutIdleFunc(idle);

		fprintf(stderr, "Open GL version %s\n", glGetString(GL_VERSION));
		init_gl(argc, argv); 


		glutMainLoop();
	} else {
		printf("Selected grid size (%d) or arena size (%d) are too small!",
				grid_size, arena_size);
	} 

	return 0; 
}