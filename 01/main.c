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

#include <GL/glew.h>
#include <gtk/gtk.h>

static void on_realize(GtkGLArea *area);
static void on_render(GtkGLArea *area, GdkGLContext *context);

GLuint program;
GLuint vao, vbo_triangle;
GLint attribute_coord2d;

int main(int argc, char *argv[]) {

	GtkWidget *window;
	GtkWidget *glArea;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "DashGL - Shooter");
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);
	gtk_window_set_type_hint(GTK_WINDOW(window), 5);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	glArea = gtk_gl_area_new();
	gtk_widget_set_vexpand(glArea, TRUE);
	gtk_widget_set_hexpand(glArea, TRUE);
	g_signal_connect(glArea, "realize", G_CALLBACK(on_realize), NULL);
	g_signal_connect(glArea, "render", G_CALLBACK(on_render), NULL);
	gtk_container_add(GTK_CONTAINER(window), glArea);

	gtk_widget_show_all(window);

	gtk_main();

	return 0;

}

static void on_realize(GtkGLArea *area) {

	g_print("on realize\n");

	gtk_gl_area_make_current(area);
	if(gtk_gl_area_get_error(area) != NULL) {
		fprintf(stderr, "Unknown Error\n");
		return;
	}


	g_print("00\n");

	glewExperimental = GL_TRUE;
	glewInit();

	g_print("01\n");

	const GLubyte *renderer = glGetString(GL_RENDERER);
	const GLubyte *version = glGetString(GL_VERSION);
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	gtk_gl_area_set_has_depth_buffer(area, TRUE);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	g_print("02\n");

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLfloat triangle_vertices[] = {
		 0.0,  0.8,
		-0.8, -0.8,
		 0.8, -0.8
	};
	glGenBuffers(1, &vbo_triangle);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_triangle);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(triangle_vertices),
		triangle_vertices,
		GL_STATIC_DRAW
	);

	g_print("03\n");

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glDisableVertexAttribArray(0);

	GLint compile_ok = GL_FALSE;
	GLint link_ok = GL_FALSE;

	g_print("04\n");

	const char *vs_source = 
	"#version 130\n"
	"attribute vec2 coord2d;\n"
	"void main(void) {\n"
	"	gl_Position = vec4(coord2d, 0.0, 1.0);\n"
	"}";

	const char *fs_source = 
	"#version 130\n"
	"void main (void) {\n"
	" gl_FragColor = vec4(0.0, 0.0, 1.0, 1.0);\n"
	"}";

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fs_source, NULL);
	glCompileShader(fs);
	glGetShaderiv(fs, GL_COMPILE_STATUS, &compile_ok);
	if(!compile_ok) {
		fprintf(stderr, "Error in fragment shader\n");
		return;
	}

	g_print("05\n");

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vs_source, NULL);
	glCompileShader(vs);
	glGetShaderiv(vs, GL_COMPILE_STATUS, &compile_ok);
	if(!compile_ok) {
		fprintf(stderr, "Error in vertex shader\n");
		return;
	}

	g_print("06\n");

	program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
	if(!link_ok) {
		fprintf(stderr, "Error when linking program\n");
		return;
	}

	g_print("07\n");

	const char *attribute_name = "coord2d";
	attribute_coord2d = glGetAttribLocation(program, attribute_name);
	if(attribute_coord2d == -1) {
		fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
		return;
	}
	
}

static void on_render(GtkGLArea *area, GdkGLContext *context) {

	g_print("on render\n");
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(program);

	glBindVertexArray(vao);
	glEnableVertexAttribArray(attribute_coord2d);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_triangle);
	glVertexAttribPointer(
		attribute_coord2d,
		2,
		GL_FLOAT,
		GL_FALSE,
		0,
		0
	);

	glDrawArrays(GL_TRIANGLES, 0, 3);
	glDisableVertexAttribArray(attribute_coord2d);


}



