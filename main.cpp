#ifdef __APPLE__
#include <GLUT/glut.h> 
#else
#include <GL/glew.h>
#include <GL/glut.h> 
#endif

#ifdef _linux_
unsigned int delay_step = 1; // Tick difference between difficulties
unsigned int max_delay = 23; // The largest delay (in ticks) between snake steps
unsigned int menu_ticks = 8; // ticks before the camera should rotate in menu screen
#else
unsigned int delay_step = 10; // Tick difference between difficulties
unsigned int max_delay = 120; // The largest delay (in ticks) between snake steps
unsigned int menu_ticks = 50; // ticks before the camera should rotate in menu screen
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string>
#include <math.h>
#include "grid.h"
#include "snake.h"
#include "pellet.h"
#include "buttonlist.h"
#include "load_and_bind_texture.h"

// Tex values are used to access bound textures from the 'textures' array
enum tex { INTER=0, HEADFRONT=1, HEADRIGHT=2, HEADLEFT=3, HEADTOP=4,
		   SEGMENT=5, SEGMENTT=6, TURNL=7, TURNR=8,
		   TAILR=9, TAILL=10, TAILT=11, END=12,
		   GRASS=13, DIRT=14, APPLE=15, APPLETOP=16, TEXNUM=17 };
// These values are used to intuitively access coordinates for the
// Left, Right etc. sides of a 1x1 cube, declared further down
enum cube_side { L=0, R=1, T=2, B=3, F=4, N=5 };

float arena_size = 600.0f;	// The size, in world units, of the play area
float screen_pad = 5.0f;	// In world coordinates/sizes
float hud_height = 50.0f;	// The height of the HUD in the world
float y_pos = .0f;			// The Y position of the camera
float x_pos = .0f;			// The X position of the camera
float z_pos = 400.0f;		// the Z position of the camera
float x_ref = .0f;			// The X position of the camera reference point
float y_ref = .0f;			// The Y position of the camrea reference point
float z_ref = .0f;			// The Z position of the camera reference point

// The amount by which the grass brock extends past the arena size
float extend = 100.0f;
float text_size = .2f;
float camlerp = .0f;		// Used to smoothly rotate the camera
float plank_pad = 5.0f;		// The padding between the planks of the fence
float plank_th = 10.0f;		// The thickness of the planks of the fence

// The distance between the camera and the reference point in First Person mode
float horizon = 100.0f;
float plank_size;		// The width of the planks of the fence;
						// this size depends on the size of the arena
						
// The angle of rotation of the camera around the origin while in Main Menu
float cam_angle;
float screen_height;	// The total height of the viewport and screen
float screen_width;		// The total width of the viewport and screen
float h_limit;			// The horizontal limit at which can draw
float v_limit;			// The vertical limit at which can draw
float grid_left;		// The left edge of the grid (x-coordinate)
float grid_right;		// The right edge of the grid (x-coordinate)
float grid_top;			// The top edge of the grid (y-coordinate)
float grid_bot;			// The bottom edge of the grid (y-coordinate)
float view_rad;			// The radius of the circle followed by the spinning camera

// Cube edges: Right, Left etc.
float c_r = 1.0f;
float c_l = .0f;
float c_t = .0f;
float c_b = -1.0f;
float c_f = .0f;
float c_n = 1.0f;

// Coordinates of cube corners divided into sides.
// Duplicated values allow usage as vectors of size 3 to draw each vertex.
// Further duplication allows drawing side vertices sequentially.
static float cube[6][4][3] = {
	{{c_l, c_t, c_n}, {c_l, c_b, c_n}, {c_l, c_b, c_f}, {c_l, c_t, c_f}}, // Left
	{{c_r, c_b, c_n}, {c_r, c_t, c_n}, {c_r, c_t, c_f}, {c_r, c_b, c_f}}, // Right
	{{c_r, c_t, c_n}, {c_l, c_t, c_n}, {c_l, c_t, c_f}, {c_r, c_t, c_f}}, // Top
	{{c_l, c_b, c_n}, {c_r, c_b, c_n}, {c_r, c_b, c_f}, {c_l, c_b, c_f}}, // Bottom
	{{c_l, c_b, c_f}, {c_l, c_t, c_f}, {c_r, c_t, c_f}, {c_r, c_b, c_f}}, // Far
	{{c_l, c_t, c_n}, {c_r, c_t, c_n}, {c_r, c_b, c_n}, {c_l, c_b, c_n}}  // Near
};

static float c_normals[6][3] = {
	{-1.0f, .0f, .0f},
	{1.0f, .0f, .0f},
	{.0f, 1.0f, .0f},
	{.0f, -1.0f, .0f},
	{.0f, .0f, -1.0f},
	{.0f, .0f, 1.0f}
};
// Properties of grass material
float g_ambient[] = {0.4, 0.6, 0.4, 1.0};
float g_diffuse[] = {0.5, 0.5, 0.5, 1.0};
float g_specular[] = {.5f, .5f, .5f, 1.0};
float g_shininess[] = {40.0};

// Properties of snake material
float s_ambient[] = {0.7, 0.7, 0.7, 1.0};
float s_diffuse[] = {0.75, 0.75, 0.75, 1.0};
float s_specular[] = {1.0, 1.0, 1.0, 1.0};
float s_shininess[] = {70.0};

// Properties of wood material
float w_ambient[] = {0.5, 0.5, 0.5, 1.0};
float w_diffuse[] = {.4, .4, .4, 1.0};
float w_specular[] = {.4, .4, .4, 1.0};
float w_shininess[] = {40.0};

// Properties of pellet material
float p_ambient[] = {0.7, 0.6, 0.4, 1.0};
float p_diffuse[] = {0.6, 0.6, 0.6, 1.0};
float p_specular[] = {1.0, 1.0, 1.0, 1.0};
float p_shininess[] = {55.0};

// Light properties
float light_ambient[] = {1.0f, 1.0f, 1.0f, 1.0f};
float light_diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};

float lightpos = 0.0f;

// Coordinates if texture corners. All textures are individually drawn,
// Square and fully used
static int tex_source_coords[4][2] {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

bool menu;		// If game is in menu
bool running;	// If game is running at the moment
bool game_over;	// If the game has been lost
bool moved;		// If the snake has moved since the last direction change
bool loop;		// If the snake is allowed to loop at edges of screen
bool display_grid;	// If the grid should be drawn
bool invisible;		// If the snake's middle-body should be drawn;
					// This allows for an interesting game mode where the
					//  player must keep track of where they have been

bool fp;			// If the game should be played in First Person mode
int ticks;			// Ticks that have been counted. Resets depending on difficulty
int menu_screen; // The current menu screen (check Destination in button.h for options)
unsigned int grid_size = 20;	  // The order of the grid/matrix. Must be >5
unsigned int difficulty_step = 2; // The required score change for difficulty increase
unsigned int max_difficulty = 9;
unsigned int g_bitmap_text_handle = 0;
unsigned int y_up = 0; // The Y coordinate of the camera up vector
unsigned int z_up = 1; // The Z coordinate of the camera up vector
unsigned int plank_num = 10; // The number of fence planks on one side of the fence
unsigned int difficulty; 	 // The current difficulty level
unsigned int textures[TEXNUM];	// Stores bound texture handles
Grid* grid;				// Stores grid cell coordinates
Snake* snake;			// Stores snake information and allows snake movement
Pellet* pellet;			// Stores food pellet info and provides pellet functionality
ButtonList* buttonList;	// Stores GUI buttons and their information/effects

/**
 * Returns a text handle from generating Times New Roman bitmap characters
 */
unsigned int make_bitmap_text() {
	unsigned int handle_base = glGenLists(256); 
	for (int i = 0; i < 256; i++) {
		// a new list for each character
		glNewList(handle_base + i, GL_COMPILE);
			glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_10, i);
		glEndList();
	}
	return handle_base;
}

/**
 * Binds textures and stores handles to be used when drawing
 * All textures used here are 900x900 pixels and are drawn using
 *  the initialized tex_source_coord source coordinates
 */
void load_and_bind_textures()
{
	textures[INTER] = load_and_bind_texture("./images/inter.png");
	textures[HEADFRONT] = load_and_bind_texture("./images/headfront.png");
	textures[HEADRIGHT] = load_and_bind_texture("./images/headright.png");
	textures[HEADLEFT] = load_and_bind_texture("./images/headleft.png");
	textures[HEADTOP]  = load_and_bind_texture("./images/headtop.png");
	textures[SEGMENT]  = load_and_bind_texture("./images/segment.png");
	textures[SEGMENTT] = load_and_bind_texture("./images/segmentt.png");
	textures[TURNL] = load_and_bind_texture("./images/turnl.png");
	textures[TURNR] = load_and_bind_texture("./images/turnr.png");
	textures[TAILR] = load_and_bind_texture("./images/tailr.png");
	textures[TAILL] = load_and_bind_texture("./images/taill.png");
	textures[TAILT] = load_and_bind_texture("./images/tailt.png");
	textures[END] = load_and_bind_texture("./images/end.png");

	textures[GRASS] = load_and_bind_texture("./images/grass.png");
	textures[DIRT]  = load_and_bind_texture("./images/dirt.png");
	textures[APPLE] = load_and_bind_texture("./images/apple.png");
	textures[APPLETOP] = load_and_bind_texture("./images/appletop.png");
}

/**
 * Sets ortographic projection, saving the current matrix
 * This is used to draw an immovable HUD and the GUI elements
 *  on top of the view-relative game objects
 */
void setOrthographicProjection() {
	glMatrixMode(GL_PROJECTION);
	// Save the perspective projection matrix
	glPushMatrix();
	// Set the 2D orthographic perspective
	glLoadIdentity();
	gluOrtho2D(-h_limit, h_limit, -v_limit, v_limit);
	glMatrixMode(GL_MODELVIEW);
}

/**
 * Restores the saved matrix after setOrtographicProjection
 *  has been used, and the effects no longer have use
 * This is used to resume regular game object drawing once
 *  HUD and GUI elements have been drawn
 */
void restorePerspectiveProjection() {
	glMatrixMode(GL_PROJECTION);
	// Revert to the perspective projection
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

/**
 * Frees memory and exits the application
 */
void quit_game() {
	grid->Delete();
	snake->Delete();
	buttonList->Delete();
	delete grid;
	delete snake;
	delete buttonList;
	exit(0);
}

/**
 * Repositions the camera in a first person manner, on top of the snake,
 *  pointing ahead. Only called if the fp flag is set.
 */
void move_fp() {
	// Retrieve the head coordinates and assume this is the camera position
	unsigned int* head = snake->GetHeadPosition();
	Cell* new_cell = grid->GetCellAt(head[0], head[1]);
	float size = grid->GetCellSize();
	x_pos = new_cell->GetX() + (size/2);
	y_pos = new_cell->GetY() - (size/2);

	// Based on the direction faced, the camera is moved back, so as to
	//  see the snake head, then the reference point is set ahead of the snake
	switch(snake->GetDirection()) {
		case UP:	x_ref = x_pos;
					y_ref = y_pos + horizon;
					y_pos -= size;
					break;
		case DOWN:	x_ref = x_pos;
					y_ref = y_pos - horizon;
					y_pos += size;
					break;
		case LEFT:	x_ref = x_pos - horizon;
					y_ref = y_pos;
					x_pos += size;
					break;
		case RIGHT:	x_ref = x_pos + horizon;
					y_ref = y_pos;
					x_pos -= size;
					break;
	}
}

void stop_game() {
	game_over = true;
	running = false;
	buttonList->Refresh();
	buttonList->AddButton("Main menu", QMAIN);
	buttonList->AddButton("Quit", QUIT);
}

/**
 * Returns the width of a string if printed in screen.
 * Move to util.
 */
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

// Displays text.
// Move to util
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
/**
 * Draws the text at the top-middle of the screen.
 * This is used to indicate the state of the game.
 */
void draw_header(const char* text) {
	glPushMatrix();
		// Center the text and move it to the top, adding appropriate padding
		float h_center_offset = -str_width(text) / ( 2 / text_size );
		float v_center_offset = v_limit - screen_pad - ( hud_height / 2 );
		glTranslatef(.0f + h_center_offset, .0f + v_center_offset, .0f);
		glScalef(text_size, text_size, 1.0f);
		draw_text(text);
	glPopMatrix();
}

/**
 * Used to select the current state if the game is running,
 *  then display it using the predefined header drawing method.
 */
void draw_state() {
	const char* text;
	if(game_over) {
		text = "Game Over!";
	} else {
		text = "Eat the pellets!";
	}
	draw_header(text);
}

/**
 * Displays the current score in the upper-left corner of the screen.
 */
void draw_score() {
	// Construct the display string
	std::string text("Score: ");
	text.append(std::to_string(snake->GetScore()));

	glPushMatrix();
		// Move text to corner and pad above
		float h_center_offset = -h_limit + screen_pad;
		float v_center_offset = v_limit - screen_pad - ( hud_height / 2 );
		glTranslatef(.0f + h_center_offset, .0f + v_center_offset, .0f);
		glScalef(text_size, text_size, 1.0f);
		draw_text(text);
	glPopMatrix();
}

/**
 * Translates x from the range a1-a2 to the range b1-b2.
 * Move to util.
 */
int normalize(int x, int a1, int a2, int b1, int b2) {
	return b1 + ( (x - a1) * (b2 - b1) / (a2 - a1) );
}

/**
 * Draws a square of side size 1.0, at origin.
 * Scaling on x or y by a number N will result in a respective side of size N.
 * Origin relative to square:
 *   O___________
 *  |            |
 *  |            |
 *  |            |
 *  |            |
 *  |____________|
 */
void draw_square() {
	static float vertex[4][3] = {
		{.0f, .0f, .0f},
		{1.0f, .0f, .0f},
		{1.0f, -1.0f, .0f},
		{.0f, -1.0f, .0f}
	};

	glBegin(GL_LINE_LOOP);
		for(int i = 0; i < 4; i++) {
			glVertex3fv(vertex[i]);
		}
	glEnd();
}

/**
 * Draws a cube of side size 1.0 sitting on top of the Z=0 plane.
 * Scaling on an axis by N will result of a respective side of size N.
 * Origin relative to cube (showing face resting on Z=0 plane):
 *   O___________
 *  |            |
 *  |            |
 *  |            |
 *  |            |
 *  |____________|
 */
void draw_cube() {
	// For each side
	for(size_t i = 0; i < 6; i++) {
		glBegin(GL_QUADS);
			// Retrieve side vertices
			for(size_t j = 0; j < 4; j++) {
				glVertex3fv(cube[i][j]);
			}
			glNormal3fv(c_normals[i]);	// Retrieve side normal			
		glEnd();
	}
}

/**
 * Draws a cube side (L=Left, R=Right etc.) using the texture found at
 *  the specified source location in the 'textures' array.
 * Uses the coordinates described in the 'cube' array to draw each side.
 */
void draw_textured_side(tex source, cube_side side) {
	// Pick texture
	glBindTexture(GL_TEXTURE_2D, textures[source]);
	glBegin(GL_QUADS);
		// Draw vertices and texture
		for(size_t i = 0; i < 4; i++) {
			glTexCoord2f(tex_source_coords[i][0],
						 tex_source_coords[i][1]);
			glVertex3fv(cube[side][i]);
		}
		glNormal3fv(c_normals[side]);	// Retreieve side normal
	glEnd();
}

/**
 * Draws cube side as a top, considering direction in which the cube would be
 *  facing by drawing the vertices in a different order for each orientation.
 * The top is textured using the texture found at the specified location in
 *  the 'textures' array.
 */
void draw_textured_top(tex source, cube_side side, unsigned int dir) {
	glBindTexture(GL_TEXTURE_2D, textures[source]);
	glBegin(GL_QUADS);
		unsigned int add = 1; // Number of times rotated
		switch(dir) {
			case LEFT: add = 3; break;
			case UP: add = 0; break;
			case DOWN: add = 2; break;						
		}
		for(size_t i = 0; i < 4; i++) {
			glTexCoord2f(tex_source_coords[i][0],
						 tex_source_coords[i][1]);
			glVertex3fv(cube[side][(i+add)%4]);
		}
		glNormal3fv(c_normals[side]);	// Retrieve side normal
	glEnd();
}

/**
 * Draws the cube of grass abd dirt on which the arena rests.
 */
void draw_grass() {
	// set the surface properties (all same material)
	glMaterialfv(GL_FRONT, GL_AMBIENT, g_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, g_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, g_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, g_shininess);
	glPushMatrix();
		// Calculate size from arena size and desired extra padding
		float size = grid_right - grid_left + (2*extend);
		glTranslatef(grid_left - extend, grid_top + extend, -size);
		glScalef(size, size, size);

		// Draw the textured Dirt sides, and the Grass top
		glEnable(GL_TEXTURE_2D);
			draw_textured_side(DIRT, L);
			draw_textured_side(DIRT, R);
			draw_textured_side(DIRT, T);
			draw_textured_side(DIRT, B);
			draw_textured_top(GRASS, N, RIGHT);
		glDisable(GL_TEXTURE_2D);				
	glPopMatrix();
}

/**
 * Method that draws a grid using the coordinates stored in the Grid structure. 
 */
void draw_grid() {
	glDisable(GL_LIGHTING);
	for(int i = 0; i < grid_size; i++) {
		for(int j = 0; j < grid_size; j++) {
			glPushMatrix();
				// Extract coordinates and move sqare into position
				Cell* new_cell = grid->GetCellAt(i, j);
				glTranslatef(new_cell->GetX(), new_cell->GetY(), 1.0f);
				// Scale the square to represent cell size
				glScalef(grid->GetCellSize(), grid->GetCellSize(), 1.0f);
				draw_square();
			glPopMatrix();
		}
	}
	glEnable(GL_LIGHTING);
}

/**
 * Draws the textured head of the snake.
 */
void draw_head(unsigned int dir) {
	/** 
	 * The cube side order determines on which side of the head each
	 *  texture should be drawn.
	 * 1. INTER, 2. Front, 3. RIGHT, 4. LEFT, 5. TOP
	 * Therefore, the order values are rearranged such that the contained
	 *  value (L=Left etc.) represents the desired location of the texture
	 */
	cube_side order[5] = {L, R, B, T, N};
	switch(dir) {
		case LEFT: order[0] = R; order[1] = L;
				   order[2] = T; order[3] = B; break;
		case UP: order[0] = B; order[1] = T;
				 order[2] = R; order[3] = L; break;
		case DOWN: order[0] = T; order[1] = B;
				   order[2] = L; order[3] = R; break;
	}
	glEnable(GL_TEXTURE_2D);
		// The first 4 textures are ordered
		for(size_t i = 0; i < 4; i++) {
			draw_textured_side((tex) i, order[i]);
		}
		draw_textured_top(HEADTOP, order[4], dir);
	glDisable(GL_TEXTURE_2D);
}

/**
 * Draws the textured tail of the snake.
 */
void draw_tail(unsigned int dir) {
	/** 
	 * The cube side order determines on which side of the tail each
	 *  texture should be drawn.
	 * 1. INTER, 2. END, 3. RIGHT, 4. LEFT, 5. TOP
	 * Therefore, the order values are rearranged such that the contained
	 *  value (L=Left etc.) represents the desired location of the texture
	 */
	cube_side order[5] = {R, L, B, T, N};
	switch(dir) {
		case LEFT: order[0] = L; order[1] = R;
					order[2] = T; order[3] = B; break;
		case UP: order[0] = T; order[1] = B;
					order[2] = R; order[3] = L; break;
		case DOWN: order[0] = B; order[1] = T;
					order[2] = L; order[3] = R; break;
	}
	glEnable(GL_TEXTURE_2D);
		draw_textured_side(INTER, order[0]);
		draw_textured_side(END, order[1]);
		draw_textured_side(TAILR, order[2]);
		draw_textured_side(TAILL, order[3]);
		draw_textured_top(TAILT, order[4], dir);
	glDisable(GL_TEXTURE_2D);
}

/**
 * Draws a textured segment of the snake where a junction exists.
 */
void draw_turn(unsigned int ahead, unsigned int behind) {
	/** 
	 * The cube side order determines on which side of the segment each
	 *  texture should be drawn.
	 * 1. SIDE, 2. INTER, 3. SIDE, 4. INTER, 5. TOP.
	 * Therefore, the order values are rearranged such that the contained
	 *  value (L=Left etc.) represents the desired location of the texture.
	 * Here, the direction of each of the juction ends matters.
	 * 'ahead' (the directin of the segment afead of this one) determines the
	 *  first 2 values, and 'behind' determines the second and third.
	 */
	cube_side order[5] = {L, R, B, T, N};
	switch(ahead) {
		case LEFT: order[0] = R; order[1] = L; break;
		case UP: order[0] = B; order[1] = T; break;
		case DOWN: order[0] = T; order[1] = B; break;
	}
	switch(behind) {
		case LEFT: order[2] = L; order[3] = R; break;
		case RIGHT: order[2] = R; order[3] = L; break;
		case UP: order[2] = T; order[3] = B; break;
	}
	glEnable(GL_TEXTURE_2D);
		draw_textured_side(SEGMENT, order[0]);
		draw_textured_side(INTER, order[1]);
		draw_textured_side(SEGMENT, order[2]);
		draw_textured_side(INTER, order[3]);
		// Based on the relation, determine if the junction is a left or right turn
		if((ahead > behind || (ahead == UP && behind == LEFT)) &&
			!(ahead == LEFT && behind == UP)) {
			draw_textured_top(TURNL, order[4], ahead);
		} else {
			draw_textured_top(TURNR, order[4], ahead);
		}
	glDisable(GL_TEXTURE_2D);
}

/**
 * Draws a straight textured segment of the snake.
 */
void draw_segment(unsigned int dir) {
	/** 
	 * The cube side order determines on which side of the tail each
	 *  texture should be drawn.
	 * 1. INTER, 2. END, 3. RIGHT, 4. LEFT, 5. TOP
	 * Therefore, the order values are rearranged such that the contained
	 *  value (L=Left etc.) represents the desired location of the texture
	 */
	cube_side order[5] = {L, R, B, T, N};
	switch(dir) {
		case LEFT: order[0] = R; order[1] = L;
				   order[2] = T; order[3] = B; break;
		case UP: order[0] = B; order[1] = T;
				 order[2] = R; order[3] = L; break;
		case DOWN: order[0] = T; order[1] = B;
				   order[2] = L; order[3] = R; break;
	}
	glEnable(GL_TEXTURE_2D);
		draw_textured_side(INTER, order[0]);
		draw_textured_side(INTER, order[1]);
		draw_textured_side(SEGMENT, order[2]);
		draw_textured_side(SEGMENT, order[3]);		
		draw_textured_top(SEGMENTT, order[4], dir);
	glDisable(GL_TEXTURE_2D);
}

/**
 * Draws the 3d snake, distinguishing between the head, tail, straight
 *  segments, and turning segments.
 */
void draw_3D_snake() {
	// set the surface properties (all same material)
	glMaterialfv(GL_FRONT, GL_AMBIENT, s_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, s_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, s_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, s_shininess);
	// At each position, elements 0 and 1 are coordinates, 2 is the
	//  direction and 3 is the previous direction
	unsigned int** positions = snake->GetSnakePosition();
	for(int i = 0; i < snake->GetLength(); i++) {
		glPushMatrix();
			// Move into position and scale to cell size
			float size = grid->GetCellSize();
			Cell* new_cell = grid->GetCellAt(positions[i][0], positions[i][1]);
			glTranslatef(new_cell->GetX(), new_cell->GetY(), .0f);
			glScalef(size, size, size);
			if(i == 0) {
				// Snake head
				draw_head(positions[i][2]);
			} else if(i == snake->GetLength() - 1) {
				// Snake tail
				draw_tail(positions[i][2]);
			} else if(!invisible) {
				if(positions[i][2] != positions[i][3]) {
					// Turning segment
					draw_turn(positions[i][2], positions[i][3]);
				} else {
					// Straight segment
					draw_segment(positions[i][2]);
				}
			}
		glPopMatrix();
	}

	// Free memory allocated when retrieving positions
	for ( int i = 0; i < snake->GetLength(); i++) {
		delete [] positions[i];
	}
	delete [] positions;
}

/**
 * Draws the collectible pellet, textured as an apple
 */
void draw_3D_pellet() {
	// Set the surface properties (all same material)
	glMaterialfv(GL_FRONT, GL_AMBIENT, p_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, p_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, p_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, p_shininess);
	Cell* new_cell = grid->GetCellAt(pellet->GetY(), pellet->GetX());
	glPushMatrix();
		float size = grid->GetCellSize();
		// Move into position and scale to cell size
		glTranslatef(new_cell->GetX(), new_cell->GetY(), .0f);
		glScalef(size, size, size);
		glEnable(GL_TEXTURE_2D);
			draw_textured_side(APPLE, L);
			draw_textured_side(APPLE, R);
			draw_textured_side(APPLE, T);
			draw_textured_side(APPLE, B);
			draw_textured_top(APPLETOP, N, RIGHT);
		glDisable(GL_TEXTURE_2D);
	glPopMatrix();
}

/**
 * Draws a plank, an element of the fence outline
 */
void draw_plank() {
	// Set the surface properties (all same material)
	glMaterialfv(GL_FRONT, GL_AMBIENT, w_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, w_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, w_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, w_shininess);
	glPushMatrix();
		glScalef(plank_th, plank_size, 100.0f);
		glColor3f(0.5f, 0.35f, 0.05f);
		draw_cube();
	glPopMatrix();
}

/**
 * Draws a brown fence outline made of planks
 */
void draw_outline() {
	glPushMatrix();
		// Move to point of first plank
		glTranslatef(grid_left - screen_pad - plank_th,
					 grid_bot - (plank_size / 2), .0f);
		// For each side, draw the planks, then rotate, and repeat
		for(size_t i = 1; i <= 4; i++) {
			for(size_t j = 1; j <= plank_num; j++) {
				glTranslatef(.0f, plank_size + plank_pad, .0f);
				draw_plank();
			}
			glRotatef(-90.0f, .0f, .0f, 1.0f);
		}
	glPopMatrix();
}

/**
 * Displays the GUI elements: header, score, and buttons
 */
void display_gui() {
	glDisable(GL_LIGHTING);
	glColor3f(1.0f, .0f, .0f);	// Red
	glPushMatrix();
		glTranslatef(.0f, .0f, 1.0f);

		glMatrixMode(GL_MODELVIEW);
		if(menu_screen == MAIN) {
			draw_header("Main Menu");
		} else if(menu_screen == OPTIONS) {
			draw_header("Options");
		} else if(menu_screen == GAME) {
			draw_state();
			draw_score();
		} else if(menu_screen == QUIT) {
			quit_game();
		} else if(menu_screen == PAUSE) {
			draw_header("Pause");
		}
		buttonList->DrawButtons();
	glPopMatrix();
	// Set colour to white to not affect other objects
	glColor3f(1.0f, 1.0f, 1.0f);
	glEnable(GL_LIGHTING);
}

/**
 * Displays the game objects
 */
void display_game() {
	glEnable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	// Camera is moved either above the arena, or positioned on the
	//  back of the snake (FP mode)
	gluLookAt(x_pos, y_pos, z_pos, // eye position
			  x_ref, y_ref, z_ref, // reference point
			  0, y_up, z_up        // up vector
		);
	draw_grass();	// Draw the arena background
	// Draw the grid on which the snake and pellets will be displayed
	if(display_grid) {
		draw_grid();
	}
	// Draw the snake
	draw_3D_snake();
	// Draw the pellet
	draw_3D_pellet();
	// Draw arena outline fence
	draw_outline();
	glDisable(GL_DEPTH_TEST);
}


/**
 * Calls display methods to draw all game elements
 */
void display() {
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	display_game();	// Display the game using the curent projection matrix

	// Display immovable GUI elemtns, saving current state for game objects
	setOrthographicProjection();
		glPushMatrix();
		glLoadIdentity();
		display_gui();
		glPopMatrix();
	restorePerspectiveProjection();

	glutSwapBuffers();
}

/**
 * Initialises the grid structure using the latest calculated parameters
 */
void init_grid() {
	// If already initialised, clear
	if(grid) {
		grid->Delete();
		delete grid;
	}
	grid = new Grid(arena_size, grid_size, screen_pad,
					grid_left,
					grid_top);

	// Keep track of arena edges which depend on cell size
	grid_right = grid_left + (grid_size * grid->GetCellSize());
	grid_bot = grid_top - (grid_size * grid->GetCellSize());
}

/**
 * Initialises the snake structure
 */
void init_snake() {
	// If already initialised, clear
	if(snake) {
		snake->Delete();
		delete snake;
	}
	// Here, both parameters may change between calls to init_snake
	snake = new Snake(3, 2, grid_size, loop);
}

/**
 * Initialises the pellet object
 */
void init_pellet() {
	// If already initialised, clear
	if(pellet) {
		pellet->Delete();
		delete pellet;
	}
	int pelletX = (int) ( rand() % ( grid_size - 1 ));
	int pelletY = (int) ( rand() % ( grid_size - 1 ));
	pellet = new Pellet(pelletX, pelletY);
}

/**
 * Calls methods to initialise data structures used to store game state
 */
void init_structs() {
	init_grid();
	init_snake();
	init_pellet();
	ticks = 0;
}

/**
 * Initialises all aspects of the game state.
 * Should only be called once, at the start.
 */
void init_state() {
	menu = true;		// If game is in menu
	running = false;	// If game is running at the moment
	game_over = false;	// If the game has been lost
	moved = false;		// If the snake has moved since the last direction change
	loop = true;		// If the snake is allowed to loop at edges of screen
	display_grid = false; // If the grid should be drawn
	invisible = false;	// If the snake's middle-body should be drawn
	fp = false; // If the game should be displayed in First Person mode
	difficulty = 0;	// The start difficulty (0=lowest, max_difficulty=highest)
	screen_height = arena_size + screen_pad;	// The desired screen height
	screen_width = arena_size + screen_pad;		// The desired screen width
	h_limit = screen_width / 2;  // The horizontal drawing limit to the left/right
	v_limit = screen_height / 2; // The vertical drawing limit up/down
	grid_left = -h_limit + screen_pad;	// The leftmost limit of the arena grid
	grid_top = v_limit - screen_pad;	// The top limit of the arena grid
	// The radius of the circular trajectory of the camera
	view_rad = grid_right - grid_left;
	cam_angle = .0f; // The rotation angle of the camera
	
	init_structs();	// Initialise the data structures which store the game state
	
	// The size of the planks which make up the fence outline
	plank_size = (grid_right - grid_left) / plank_num;

	srand(time(NULL));	// Initialise seed
	
	menu_screen = MAIN;	// Set menu screen to Main Menu

	// The button list structure should only be initialised once
	// Therefore, it is not included in the init_structs method
	int v_center_offset = v_limit - hud_height - screen_pad;
	buttonList = new ButtonList(0 + v_center_offset, screen_width, 40);
	buttonList->AddButton("Play", GAME);
	buttonList->AddButton("Options", OPTIONS);
	buttonList->AddButton("Quit", QUIT);
}

/**
 * Initialise the Viewport, Projection, textures and text bitmaps
 */
void init_gl(int argc, char* argv[]) {
	glViewport(0, 0, screen_width, screen_height);
	glMatrixMode(GL_PROJECTION); 
	glLoadIdentity();
	gluPerspective(90.0f, screen_width / screen_height, 1.0f, 800.0f);
	
	load_and_bind_textures();
	GLenum error = glGetError();
	if (error!=GL_NO_ERROR) {
		printf("GL error %s\n", gluErrorString(error));
		fflush(stdout);
	}
	g_bitmap_text_handle = make_bitmap_text();		

	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);

	// fix the light position
	float light_position[] = {2000.0f, 2000.0f, 2000.0f, .0f};
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	// enable lighting and turn on the light0
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	// so that hidden surfaces are removed
	glEnable(GL_DEPTH_TEST);

	// mode of shading
	glShadeModel(GL_SMOOTH);
}

/**
 * Checks for collision between the head and the pellet/walls
 */
void check_head_collisions() {
	// If head collides with body
	if(snake->Bite() && running) {
		stop_game();
	}
	// If head collides with pellet
	// At each position, element 0 and 1 contain X and Y coordinates,
	//  2 contains the direction, and 3 contains the previous direction
	unsigned int** positions = snake->GetSnakePosition();
	bool onSnake = false;
	if(positions[0][0] == pellet->GetY() &&
			positions[0][1] == pellet->GetX()) {
		// The pellet should be collected
		int pelletX;
		int pelletY;
		// Generate pellet locations until it no longer occupies the
		//  same space as another object
		do{
			onSnake = false; // Assume the pellet will not spawn on top of snake
			pelletX = (int) ( rand() % ( grid_size - 1 ));
			pelletY = (int) ( rand() % ( grid_size - 1 ));
			for(size_t i = 0; i < snake->GetLength(); i++) {
				// Check if the pellet is on top of a snake segment
				if(pelletX == positions[i][1] && pelletY == positions[i][0]) {
					onSnake = true;
					break;
				}
			}
		} while(!pellet->Reposition(pelletX, pelletY) || onSnake);
		snake->EatPellet();

		// If the snake has eaten enough pellets, increase difficulty
		if(difficulty < max_difficulty && 
				snake->GetScore() >= difficulty * difficulty_step) {
			difficulty++;
		}
	}
	// Release memory allocated during checks
	for ( int i = 0; i < snake->GetLength() - 1; i++) {
		delete [] positions[i];
	}
	delete [] positions;
}

/**
 * Handles mouse interactions.
 * Currently only interested in Downward, left clicks.
 * Reactions only triggered if a ButtonList button has been clicked
 */
void mouse_action(int button, int state, int x, int y) {
	if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		int** bounds = buttonList->GetButtonBounds();
		// Convert window coordinates to view coordinates
		x = normalize(x, 0, screen_width, -h_limit, h_limit);
		y = normalize(y, 0, screen_height, v_limit, -v_limit);
		// If the click was on any of the buttons, react accordingly
		int checks = buttonList->GetCount();
		for(int i = 0; i < checks; i++) {
			if(y < bounds[i][0] && y > bounds[i][1] &&
			   		x > bounds[i][2] && x < bounds[i][3]) {
				menu_screen = bounds[i][4];
				switch(menu_screen) {
					case QMAIN:
						// Used to return to main menu from pause/game over menu
						game_over = false;
						menu = true;
						menu_screen = MAIN;

						// Reset up vector to view arena while rotating
						y_up = 0;
						z_up = 1;

						// Replace the eye above arena
						x_pos = .0f;
						y_pos = .0f;
						z_pos = 400.0f;

						// Set the reference point to origin in case it was changed
						x_ref = .0f;
						y_ref = .0f;
						z_ref = .0f;
						init_structs();
					case MAIN:
					    // Used to return to main menu from sub-menus
						buttonList->Refresh();
						buttonList->AddButton("Play", GAME);
						buttonList->AddButton("Options", OPTIONS);
						buttonList->AddButton("Quit", QUIT);
						break;
					case GAME:
						// Used to start the game
						buttonList->Refresh();
						menu = false;
						running = true;
						// Position camera according to view mode
						if(!fp) {
							// Replace eye in middle
							x_pos = .0f;
							y_pos = .0f;
							z_pos = 400.0f;
							// Allow eye to look down
							y_up = 1.0f;
							z_up = .0f;
							// Set the reference at origin in case it was changed
							x_ref = .0f;
							y_ref = .0f;
							z_ref = .0f;
						} else {
							// Place camera in first person position
							y_up = .0f;
							z_up = 1.0f;
							z_pos = 100.0f;
							move_fp();
						}
						break;
					case LOOP:
						// Used to set/unset the looping option
						loop = !loop;
						snake->SetLoop(loop);
						menu_screen = OPTIONS;
						goto options;
					case GRID:
						// Used to turn on/off grid drawing
						display_grid = !display_grid;
						menu_screen = OPTIONS;
						goto options;
					case INV:
						// Used to turn on/off snake body invisibility
						invisible = !invisible;
						menu_screen = OPTIONS;
						goto options;
					case FP:
						// Used to turn on/off first person perspective
						fp = !fp;
						menu_screen = OPTIONS;
						goto options;
					case GSIZE:
						// Used to select a grid size
						grid_size += 5;
						if(grid_size > 25) {
							grid_size = 15;
						}
						init_structs();
						menu_screen = OPTIONS;
						goto options;
					case OPTIONS:
						// Used to access game options from Main Menu
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
						if(invisible) {
							buttonList->AddButton("Invisible body: ON", INV);
						} else {
							buttonList->AddButton("Invisible body: OFF", INV);
						}
						if(fp) {
							buttonList->AddButton("First person: ON", FP);
						} else {
							buttonList->AddButton("First person: OFF", FP);
						}
						if(grid_size == 15) {
							buttonList->AddButton("Grid size: 15x15", GSIZE);
						} else if(grid_size == 20) {
							buttonList->AddButton("Grid size: 20x20", GSIZE);
						} else {
							buttonList->AddButton("Grid size: 25x25", GSIZE);
						}
						break;
					case QUIT:
						// Used to quit the game
						quit_game();
						break;
					case PAUSE:
						// Not in use
						break;
				}
			}
		}

		// Release memory allocated during checks
		for ( int i = 0; i < checks; i++) {
			delete [] bounds[i];
		}
		delete [] bounds;
		glutPostRedisplay();
	}
}

/**
 * Method called when the window is reshaped
 */
void reshape(int w, int h) {
	screen_width = w;
	screen_height = h;
	h_limit = screen_width / 2;
	v_limit = screen_height / 2;
	glViewport(0, 0, screen_width, screen_height);
	glMatrixMode(GL_PROJECTION); 
	glLoadIdentity();

	gluPerspective(90.0f, w/h, 1.0f, 800.0f);
	glutPostRedisplay();
}

/**
 * Handles regular keyboard interactions
 * q quits
 * p pauses
 * f turns on/off first person view
 * y/Y move the camera in the Y axis
 * x/X move the camera in the X axis
 * i turns on/off snake body invisibility
 */
void keyboard(unsigned char key, int, int) {
    switch(key) {
        case 'q': 	quit_game(); break;   // Press q to force exit application
		case 'p':	if(!menu) {
						running = !running;
						if(!running) {
							menu_screen = PAUSE;
							buttonList->Refresh();
							buttonList->AddButton("Main Menu", QMAIN);
							buttonList->AddButton("Quit", QUIT);
						} else {
							buttonList->Refresh();
							menu_screen = GAME;
						}
					}
					break;
		case 'f':   fp = !fp;
					if(!fp) {
						y_up = 1.0f;
						z_up = .0f;
						x_pos = .0f;
						y_pos = .0f;
						z_pos = 400.0f;
						x_ref = .0f;
						y_ref = .0f;
						z_ref = .0f;
					} else {
						y_up = .0f;
						z_up = 1.0f;
						z_pos = 100.0f;
						move_fp();
					}
					break;
		case 'y': 	if(!fp) y_pos -= 10;
					if(y_pos < -200){
						y_pos = -200;
					}
					break;
		case 'Y': 	if(!fp) y_pos += 10;
					if(y_pos > 200) {
						y_pos = 200;
					}
					break;
		case 'x': 	if(!fp) x_pos -= 10;
					if(x_pos < -200) {
						x_pos = -200;
					}
					break;
		case 'X':	if(!fp) x_pos += 10;
					if(x_pos > 200) {
						x_pos = 200;
					}
					break;
		case 'i':   invisible = !invisible; break;
		case 'l':	lightpos -= 10.0f; break;
		case 'L':	lightpos += 10.0f; break;
    }
    glutPostRedisplay();
}

/**
 * Handles interactions made through special keys (here, just the arrow keys).
 * In first person, only Left and Right arrow keys can be used.
 * Otherwise, all 4 can be used to turn the snake in the selected direction.
 * Turning is only allowed once per step (a change in direction must be
 * 	followed by a forward movement before the direction can be changed again).
 */
void special_keys(int key, int x, int y) {
	switch(key) {
		case GLUT_KEY_UP:
			if(!fp && moved) {
				if(snake->SetDirection(UP)) {
					moved = false;
				}
			}
			break;
		case GLUT_KEY_RIGHT:
			if(moved) {
				if(fp) {
					switch(snake->GetDirection()) {
						case UP: snake->SetDirection(RIGHT); break;
						case DOWN: snake->SetDirection(LEFT); break;
						case LEFT: snake->SetDirection(UP); break;
						case RIGHT: snake->SetDirection(DOWN); break;
					}
					moved = false;
				} else if(snake->SetDirection(RIGHT)) {
					moved = false;
				}
			}
			break;
		case GLUT_KEY_DOWN:
			if(!fp && moved) {
				if(snake->SetDirection(DOWN)) {
					moved = false;
				}
			}	
			break;
		case GLUT_KEY_LEFT:
			if(moved) {
				if(fp) {
					switch(snake->GetDirection()) {
						case UP: snake->SetDirection(LEFT); break;
						case DOWN: snake->SetDirection(RIGHT); break;
						case LEFT: snake->SetDirection(DOWN); break;
						case RIGHT: snake->SetDirection(UP); break;
					}
					moved = false;
				}else if(snake->SetDirection(LEFT)) {
					moved = false;
				}
			}
			break;
	}
}

/**
 * Runs routine idle commands
 */
void idle() {
	usleep(1000);	// Microsectonds. 1000 = 1 millisecond
	ticks++;		// Increase ticks
	if(menu && ticks == menu_ticks) {
			// Rotate the camera around the arena
			cam_angle = cam_angle < 360.0f ? cam_angle + 0.2f : .0f;
			camlerp += (cam_angle - camlerp) * 0.01f;
			x_pos = view_rad*cos(camlerp);
			y_pos = view_rad*sin(camlerp);
			ticks = 0;
	} else if(running) {
		// Move the snake at a speed relative to difficulty
		if( (ticks >= max_delay - ( difficulty * delay_step ))) {
			if(snake->Move() != -1) {
				stop_game();
			} else if(fp) {
				// If the snake was able to move, reposition camera
				move_fp();
			}
			ticks = 0;
			moved = true;	// In order to allow turning
			glutPostRedisplay();
		}
		// Check that the head is not colliding in this new position
		check_head_collisions();
	}
	glutPostRedisplay();
}

int main(int argc, char* argv[]) {
	// For a properly working game, grid order must be greater than 5
	if(grid_size > 5 && arena_size > 200) {
		init_state();
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
		glutReshapeFunc(reshape); 
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