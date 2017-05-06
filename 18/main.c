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

struct {
	vec3 pos;
	float dx;
	GLuint ship_vbo[2];
	GLuint ship_tex;
	gboolean left_down;
	gboolean right_down;
	gboolean space_down;
	short tick;
	short tick_time;
	short tick_len;
} player;

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

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

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
	player.left_down = FALSE;
	player.right_down = FALSE;
	player.space_down = FALSE;

	player.tick_time = 6;
	player.tick_len = player.tick_time / 2;
	player.tick = player.tick_time - 1;

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
	
	// End Init

	glInit = 1;

}

static void on_render(GtkGLArea *area, GdkGLContext *context) {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	int sprite;
	mat4 mvp;

	// glBindVertexArray(vao);

	// Draw Player
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, player.ship_tex);
	glUniform1i(uniform_mytexture, 0);

	glEnableVertexAttribArray(attribute_coord2d);
	glEnableVertexAttribArray(attribute_texcoord);
	
	if(player.tick > 9) {
		player.tick = 9;
	}

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

