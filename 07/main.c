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
GLint attribute_coord2d, uniform_mvp;
GtkWidget *glArea;

typedef struct {
	vec3 pos;
	gboolean active;
} Bullet;

struct {
	vec3 pos;
	float dx, dy;
	GLuint vbo[2];
	gboolean left_down;
	gboolean right_down;
	gboolean space_down;
	Bullet *bullets;
	int num_bullets;
	float bullet_radius;
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

static void on_realize(GtkGLArea *area) {

	int i;

	gtk_gl_area_make_current(area);
	if(gtk_gl_area_get_error(area) != NULL) {
		fprintf(stderr, "Unknown Error\n");
		return;
	}

	glewExperimental = GL_TRUE;
	glewInit();

	const GLubyte *renderer = glGetString(GL_RENDERER);
	const GLubyte *version = glGetString(GL_VERSION);
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	gtk_gl_area_set_has_depth_buffer(area, TRUE);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	player.pos[0] = 320.0f;
	player.pos[1] = 40.0f;
	player.pos[2] = 0.0f;
	
	player.dx = 3.0f;
	player.dy = 4.0f;
	player.num_bullets = 7;
	player.bullets = malloc(player.num_bullets * sizeof(Bullet));
	player.left_down = FALSE;
	player.right_down = FALSE;
	player.space_down = FALSE;
	player.bullet_radius = 5.0f;

	for(i = 0; i < player.num_bullets; i++) {
		player.bullets[i].active = FALSE;
	}

	GLfloat triangle_vertices[] = {
		0.0, 20.0,
		-20.0, -20.0,
		20.0, -20.0
	};
	glGenBuffers(1, &player.vbo[0]);
	glBindBuffer(GL_ARRAY_BUFFER, player.vbo[0]);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(triangle_vertices),
		triangle_vertices,
		GL_STATIC_DRAW
	);

	GLfloat bullet_vertices[] = {
		-player.bullet_radius, -player.bullet_radius,
		-player.bullet_radius, player.bullet_radius,
		player.bullet_radius, player.bullet_radius,

		player.bullet_radius, player.bullet_radius,
		player.bullet_radius, -player.bullet_radius,
		-player.bullet_radius, -player.bullet_radius,
	};

	glGenBuffers(1, &player.vbo[1]);
	glBindBuffer(GL_ARRAY_BUFFER, player.vbo[1]);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(bullet_vertices),
		bullet_vertices,
		GL_STATIC_DRAW
	);

	program = dash_create_program("sdr/vertex.glsl", "sdr/fragment.glsl");
	if(program == 0) {
		fprintf(stderr, "Program creation error\n");
		exit(1);
	}

	const char *attribute_name = "coord2d";
	attribute_coord2d = glGetAttribLocation(program, attribute_name);
	if(attribute_coord2d == -1) {
		fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
		return;
	}
	
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
	
	glUseProgram(program);
	mat4 ortho;
	mat4_orthographic(0, WIDTH, HEIGHT, 0, ortho);

	glUniformMatrix4fv(uniform_ortho, 1, GL_FALSE, ortho);
	
	glInit = 1;

}

static void on_render(GtkGLArea *area, GdkGLContext *context) {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	int i;
	mat4 mvp;

	glBindVertexArray(vao);
	glEnableVertexAttribArray(attribute_coord2d);

	glVertexAttribPointer(
		attribute_coord2d,
		2,
		GL_FLOAT,
		GL_FALSE,
		0,
		0
	);

	glBindBuffer(GL_ARRAY_BUFFER, player.vbo[1]);
	mat4_translate(player.pos, mvp);
	glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, mvp);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glEnableVertexAttribArray(attribute_coord2d);

	glVertexAttribPointer(
		attribute_coord2d,
		2,
		GL_FLOAT,
		GL_FALSE,
		0,
		0
	);

	glBindBuffer(GL_ARRAY_BUFFER, player.vbo[0]);
	for(i = 0; i < player.num_bullets; i++) {
		if(!player.bullets[i].active) {
			continue;
		}
		
		mat4_translate(player.bullets[i].pos, mvp);
		glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, mvp);
		glDrawArrays(GL_TRIANGLES, 0, 6);

	}
	glDisableVertexAttribArray(attribute_coord2d);

}

static gboolean on_idle(gpointer data) {

	int i;

	if(glInit == 0) {
		return FALSE;
	}

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

	for(i = 0; i < player.num_bullets; i++) {
		if(!player.bullets[i].active) {
			continue;
		}
		
		if(player.bullets[i].pos[1] - player.bullet_radius > HEIGHT) {
			player.bullets[i].active = FALSE;	
		}

		player.bullets[i].pos[1] += player.dy;

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

			if(!player.space_down) {
				
				player.space_down = TRUE;
				
				for(i = 0; i < player.num_bullets; i++) {
					if(player.bullets[i].active) {
						continue;
					}
					
					player.bullets[i].active = TRUE;
					
					player.bullets[i].pos[0] = player.pos[0];
					player.bullets[i].pos[1] = player.pos[1] + 5.0f;
					player.bullets[i].pos[2] = player.pos[2];

					break;
				}
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

