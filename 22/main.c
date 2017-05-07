/*
 *  This file is part of DashGL.com - Gtk - Shooter Tutorial
 *  Copyright (C) 2017 Benjamin Collins
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License version 2
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>
#include <GL/glew.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include "lib/dashgl.h"

#define WIDTH 640.0f
#define HEIGHT 480.0f
#define NUM_ENEMIES 30
#define PADDING 4.0f

static void on_realize(GtkGLArea *area);
static void on_render(GtkGLArea *area, GdkGLContext *context);
static gboolean on_idle(gpointer data);
static gint on_destroy(GtkWidget *widget);
static gboolean on_keydown(GtkWidget *widget, GdkEventKey *event);
static gboolean on_keyup(GtkWidget *widget, GdkEventKey *event);

GLuint program, glInit;
GLuint vao;
GLint attribute_coord2d, attribute_texcoord;
GLint uniform_mytexture, uniform_mvp;
GtkWidget *glArea;

typedef struct {
	vec3 pos;
	gboolean active;
	short tick;
} Bullet;

struct {
	vec3 pos;
	float dx, dy;
	GLuint ship_vbo[2];
	GLuint ship_tex;
	GLuint bullet_vbo[2];
	GLuint bullet_tex;
	Bullet *bullets;
	int num_bullets;
	float bullet_radius;
	gboolean left_down;
	gboolean right_down;
	gboolean space_down;
	short tick;
	short tick_time;
	short tick_len;
} player;

struct {
	vec3 pos[NUM_ENEMIES];
	gboolean active[NUM_ENEMIES];
	int type[NUM_ENEMIES];
	int tick[NUM_ENEMIES];
	GLuint enemy_small_tex;
	GLuint enemy_small_vbo[2];
	float radius;
	float dx, dy;
} enemies;

int main(int argc, char *argv[]) {

	GtkWidget *window;

	gtk_init(&argc, &argv);

	glInit = 0;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "DashGL - Shooter");
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);
	gtk_window_set_type_hint(GTK_WINDOW(window), 5);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(window, "key-press-event", G_CALLBACK(on_keydown), NULL);
	g_signal_connect(window, "key-release-event", G_CALLBACK(on_keyup), NULL);

	glArea = gtk_gl_area_new();
	gtk_widget_set_vexpand(glArea, TRUE);
	gtk_widget_set_hexpand(glArea, TRUE);
	g_signal_connect(glArea, "realize", G_CALLBACK(on_realize), NULL);
	g_signal_connect(glArea, "render", G_CALLBACK(on_render), NULL);
	gtk_container_add(GTK_CONTAINER(window), glArea);
	g_signal_connect(G_OBJECT(glArea), "destroy", G_CALLBACK(on_destroy), NULL);

	g_timeout_add(20, on_idle, NULL);
	gtk_widget_show_all(window);

	gtk_main();

	return 0;

}

/*****************************************************************************
 * on realize
 *****************************************************************************/

static void on_realize(GtkGLArea *area) {
	
	int i, col, row;

	// Initialize

	gtk_gl_area_make_current(area);
	if(gtk_gl_area_get_error(area) != NULL) {
		fprintf(stderr, "Unknown Error\n");
		return;
	}

	glewExperimental = GL_TRUE;
	glewInit();

	// GL Version Informagtion

	const GLubyte *renderer = glGetString(GL_RENDERER);
	const GLubyte *version = glGetString(GL_VERSION);
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	gtk_gl_area_set_has_depth_buffer(area, TRUE);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// Vertex Array Object

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create Program

	program = dash_create_program("sdr/vertex.glsl", "sdr/fragment.glsl");
	if(program == 0) {
		fprintf(stderr, "Program creation error\n");
		exit(1);
	}
	glUseProgram(program);


	// Bind Attributes

	const char *attribute_name = "coord2d";
	attribute_coord2d = glGetAttribLocation(program, attribute_name);
	if(attribute_coord2d == -1) {
		fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
		return;
	}
	
	attribute_name = "texcoord";
	attribute_texcoord = glGetAttribLocation(program, attribute_name);
	if(attribute_texcoord == -1) {
		fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
		return;
	}

	// Bind Uniforms

	const char *uniform_name = "mvp";
	uniform_mvp = glGetUniformLocation(program, uniform_name);
	if(uniform_mvp == -1) {
		fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
		return;
	}

	uniform_name = "ortho";
	GLint uniform_ortho = glGetUniformLocation(program, uniform_name);
	if(uniform_ortho == -1) {
		fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
		return;
	}
	
	uniform_name = "mytexture";
	uniform_mytexture = glGetUniformLocation(program, uniform_name);
	if(uniform_mytexture == -1) {
		fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
		return;
	}

	// Set orthographics

	mat4 ortho;
	mat4_orthographic(0, WIDTH, HEIGHT, 0, ortho);
	glUniformMatrix4fv(uniform_ortho, 1, GL_FALSE, ortho);
	
	// Player

	player.pos[0] = 320.0f;
	player.pos[1] = 24.0f;
	player.pos[2] = 0.0f;
	
	player.dx = 4.0f;
	player.dy = 4.0f;
	player.left_down = FALSE;
	player.right_down = FALSE;
	player.space_down = FALSE;

	player.tick_time = 6;
	player.tick_len = player.tick_time / 2;
	player.tick = player.tick_time - 1;

	player.num_bullets = 7;
	player.bullets = (Bullet*)malloc(player.num_bullets * sizeof(Bullet));
	player.bullet_radius = 10.0f;

	for(i = 0; i < player.num_bullets; i++) {
		player.bullets[i].active = FALSE;
	}

	// Player - Ships

	player.ship_tex = dash_texture_load("spritesheets/ship.png");
	GLfloat ship_vertices[][24] = {
		{
			-20.0, -20.0, 0.4f, 1.0f,
			-20.0,  20.0, 0.4f, 0.5f,
			 20.0,  20.0, 0.6f, 0.5f,
			 20.0,  20.0, 0.6f, 0.5f,
			 20.0, -20.0, 0.6f, 1.0f,
			-20.0, -20.0, 0.4f, 1.0f
		}, {
			-20.0, -20.0, 0.4f, 0.5f,
			-20.0,  20.0, 0.4f, 0.0f,
			 20.0,  20.0, 0.6f, 0.0f,
			 20.0,  20.0, 0.6f, 0.0f,
			 20.0, -20.0, 0.6f, 0.5f,
			-20.0, -20.0, 0.4f, 0.5f
		}
	};

	glGenBuffers(2, player.ship_vbo);

	glBindBuffer(GL_ARRAY_BUFFER, player.ship_vbo[0]);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(ship_vertices[0]),
		ship_vertices[0],
		GL_STATIC_DRAW
	);

	glBindBuffer(GL_ARRAY_BUFFER, player.ship_vbo[1]);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(ship_vertices[1]),
		ship_vertices[1],
		GL_STATIC_DRAW
	);
	
	// Player - Bullets

	player.bullet_tex = dash_texture_load("spritesheets/laser-bolts.png");
	GLfloat bullet_vertices[][24] = {
		{
			-player.bullet_radius, -player.bullet_radius, 0.0f, 1.0f,
			-player.bullet_radius,  player.bullet_radius, 0.0f, 0.5f,
			 player.bullet_radius,  player.bullet_radius, 0.5f, 0.5f,
			 player.bullet_radius,  player.bullet_radius, 0.5f, 0.5f,
			 player.bullet_radius, -player.bullet_radius, 0.5f, 1.0f,
			-player.bullet_radius, -player.bullet_radius, 0.0f, 1.0f
		}, {
			-player.bullet_radius, -player.bullet_radius, 0.5f, 1.0f,
			-player.bullet_radius,  player.bullet_radius, 0.5f, 0.5f,
			 player.bullet_radius,  player.bullet_radius, 1.0f, 0.5f,
			 player.bullet_radius,  player.bullet_radius, 1.0f, 0.5f,
			 player.bullet_radius, -player.bullet_radius, 1.0f, 1.0f,
			-player.bullet_radius, -player.bullet_radius, 0.5f, 1.0f
		}
	};

	glGenBuffers(2, player.bullet_vbo);

	glBindBuffer(GL_ARRAY_BUFFER, player.bullet_vbo[0]);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(bullet_vertices[0]),
		bullet_vertices[0],
		GL_STATIC_DRAW
	);

	glBindBuffer(GL_ARRAY_BUFFER, player.bullet_vbo[1]);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(bullet_vertices[1]),
		bullet_vertices[1],
		GL_STATIC_DRAW
	);

	// Enemies 
	
	enemies.radius = 24.0f;
	enemies.dy = -2.0f;
	enemies.dx = 1.0f;

	enemies.enemy_small_tex = dash_texture_load("spritesheets/enemy-small.png");

	for(i = 0; i < NUM_ENEMIES; i++) {
		
		col = i % 10;
		row = i / 10;

		enemies.active[i] = TRUE;
		enemies.type[i] = row;
		enemies.tick[i] = player.tick_len - 1;
		enemies.pos[i][0] = PADDING + enemies.radius + (2*enemies.radius + PADDING) * col;
		enemies.pos[i][1] = PADDING + enemies.radius + (2*enemies.radius + PADDING) * row;
		enemies.pos[i][2] = 0.0f;

	}
	
	GLfloat enemy_small_vertices[][24] = {
		{
			-enemies.radius, -enemies.radius, 0.0f, 0.0f,
			-enemies.radius,  enemies.radius, 0.0f, 1.0f,
			 enemies.radius,  enemies.radius, 0.5f, 1.0f,
			 enemies.radius,  enemies.radius, 0.5f, 1.0f,
			 enemies.radius, -enemies.radius, 0.5f, 0.0f,
			-enemies.radius, -enemies.radius, 0.0f, 0.0f
		},{
			-enemies.radius, -enemies.radius, 0.5f, 0.0f,
			-enemies.radius,  enemies.radius, 0.5f, 1.0f,
			 enemies.radius,  enemies.radius, 1.0f, 1.0f,
			 enemies.radius,  enemies.radius, 1.0f, 1.0f,
			 enemies.radius, -enemies.radius, 1.0f, 0.0f,
			-enemies.radius, -enemies.radius, 0.5f, 0.0f
		}
	};

	glGenBuffers(2, enemies.enemy_small_vbo);

	glBindBuffer(GL_ARRAY_BUFFER, enemies.enemy_small_vbo[0]);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(enemy_small_vertices[0]),
		enemy_small_vertices[0],
		GL_STATIC_DRAW
	);

	glBindBuffer(GL_ARRAY_BUFFER, enemies.enemy_small_vbo[1]);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(enemy_small_vertices[1]),
		enemy_small_vertices[1],
		GL_STATIC_DRAW
	);

	// End Init

	glInit = 1;

}

static void on_render(GtkGLArea *area, GdkGLContext *context) {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	int i, sprite;
	mat4 mvp;

	// glBindVertexArray(vao);

	// Draw Player
	
	glActiveTexture(GL_TEXTURE0);

	// Player Sprite

	glBindTexture(GL_TEXTURE_2D, player.ship_tex);
	glUniform1i(uniform_mytexture, 0);

	glEnableVertexAttribArray(attribute_coord2d);
	glEnableVertexAttribArray(attribute_texcoord);
	
	sprite = player.tick / player.tick_len;
	
	glBindBuffer(GL_ARRAY_BUFFER, player.ship_vbo[sprite]);
	
	glVertexAttribPointer(
		attribute_coord2d,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(float) * 4,
		0
	);

	glVertexAttribPointer(
		attribute_texcoord,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(float) * 4,
		(void*)(sizeof(float) * 2)
	);

	mat4_translate(player.pos, mvp);
	glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, mvp);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	// Player Bullets
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, player.bullet_tex);
	glUniform1i(uniform_mytexture, 1);

	for(i = 0; i < player.num_bullets; i++) {
		
		if(!player.bullets[i].active) {
			continue;
		}
		
		sprite = player.bullets[i].tick / player.tick_len;
		glBindBuffer(GL_ARRAY_BUFFER, player.bullet_vbo[sprite]);

		glVertexAttribPointer(
			attribute_coord2d,
			2,
			GL_FLOAT,
			GL_FALSE,
			sizeof(float) * 4,
			0
		);

		glVertexAttribPointer(
			attribute_texcoord,
			2,
			GL_FLOAT,
			GL_FALSE,
			sizeof(float) * 4,
			(void*)(sizeof(float) * 2)
		);

		mat4_translate(player.bullets[i].pos, mvp);
		glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, mvp);
		glDrawArrays(GL_TRIANGLES, 0, 6);

	}

	// Draw Enemies
	
	for(i = 0; i < NUM_ENEMIES; i++) {

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, enemies.enemy_small_tex);
		glUniform1i(uniform_mytexture, 2);

		if(!enemies.active[i]) {
			continue;
		}
		
		//sprite = player.bullets[i].tick / player.tick_len;
		glBindBuffer(GL_ARRAY_BUFFER, enemies.enemy_small_vbo[0]);

		glVertexAttribPointer(
			attribute_coord2d,
			2,
			GL_FLOAT,
			GL_FALSE,
			sizeof(float) * 4,
			0
		);

		glVertexAttribPointer(
			attribute_texcoord,
			2,
			GL_FLOAT,
			GL_FALSE,
			sizeof(float) * 4,
			(void*)(sizeof(float) * 2)
		);

		mat4_translate(enemies.pos[i], mvp);
		glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, mvp);
		glDrawArrays(GL_TRIANGLES, 0, 6);

	}

	// Disable Arrays

	glDisableVertexAttribArray(attribute_coord2d);
	glDisableVertexAttribArray(attribute_texcoord);

}

/*****************************************************************************
 * on idle
 *****************************************************************************/

static gboolean on_idle(gpointer data) {

	int i, k, move_down, shoot;
	float dx, dy, radius;

	if(glInit == 0) {
		return FALSE;
	}

	// Player Movement

	if(player.left_down) {
		player.pos[0] -= player.dx;
	}

	if(player.right_down) {
		player.pos[0] += player.dx;
	}

	if(player.pos[0] < 0.0) {
		player.pos[0] = 0.0;
	} else if(player.pos[0] > WIDTH) {
		player.pos[0] = WIDTH;
	}
	
	player.tick--;
	if(player.tick < 0) {
		player.tick = player.tick_time - 1;
	}

	// Player bullet movement
	
	for(i = 0; i < player.num_bullets; i++) {

		if(!player.bullets[i].active) {
			continue;
		}
		
		player.bullets[i].pos[1] += player.dy;
		
		if(player.bullets[i].pos[1] - player.bullet_radius > HEIGHT) {
			
			player.bullets[i].active = FALSE;

		}
		
		player.bullets[i].tick = player.bullets[i].tick - 1;

		if(player.bullets[i].tick < 0) {
			player.bullets[i].tick = player.tick_time - 1;
		}

	}

	gtk_widget_queue_draw(glArea);
	return TRUE;

}

static gint on_destroy(GtkWidget *widget) {

	g_print("Widget destroyed\n");

}

static gboolean on_keydown(GtkWidget *widget, GdkEventKey *event) {

	int i;

	switch(event->keyval) {
		case GDK_KEY_Left:
			player.left_down = TRUE;		
		break;
		case GDK_KEY_Right:
			player.right_down = TRUE;
		break;
		case GDK_KEY_space:

			if(player.space_down) {
				break;
			}	
			
			player.space_down = TRUE;
			
			for(i = 0; i < player.num_bullets; i++) {
				if(player.bullets[i].active) {
					continue;
				}
				
				player.bullets[i].active = TRUE;

				player.bullets[i].pos[0] = player.pos[0];
				player.bullets[i].pos[1] = player.pos[1];
				player.bullets[i].pos[2] = player.pos[2];
				
				player.bullets[i].tick = player.tick_time - 1;

				break;

			}

		break;
	}

}

static gboolean on_keyup(GtkWidget *widget, GdkEventKey *event) {

	switch(event->keyval) {
		case GDK_KEY_Left:
			player.left_down = FALSE;		
		break;
		case GDK_KEY_Right:
			player.right_down = FALSE;
		break;
		case GDK_KEY_space:
			player.space_down = FALSE;
		break;
	}

}
