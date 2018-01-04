// This file is part of BOINC.
// http://boinc.berkeley.edu
// Copyright (C) 2008 University of California
//
// BOINC is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// BOINC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with BOINC.  If not, see <http://www.gnu.org/licenses/>.

// Example graphics application, paired with uc2.C
// This demonstrates:
// - using shared memory to communicate with the worker app
// - reading XML preferences by which users can customize graphics
//   (in this case, select colors)
// - handle mouse input (in this case, to zoom and rotate)
// - draw text and 3D objects using OpenGL

// CMC -- NOTE: this is a chopped up version of the ss_app.cpp from BOINC,
//        used for the GridRepublic/Intel Screensaver, June 2009

#ifdef _WIN32
#include "boinc_win.h"
#else
#include <math.h>
#endif
#include <string>
#include <vector>
#ifdef __APPLE__
#include "boinc_api.h"
#include <sys/socket.h>
#endif

#include "diagnostics.h"
#include "gutil.h"
#include "boinc_gl.h"
#include "graphics2.h"
#include "txf_util.h"
#include "network.h"
#include "gui_rpc_client.h"
#include "util.h"
#include "app_ipc.h"
#include "error_numbers.h"
#include "grintelss.h"   // my PTP screensaver header

using std::string;
using std::vector;

float white[4] = {1., 1., 1., 1.};
//TEXTURE_DESC logo;
bool mouse_down = false;
int mouse_x, mouse_y;

RPC_CLIENT rpc;

bool retry_connect = true;
bool connected = false;
double next_connect_time = 0.0;

CC_STATE cc_state;
CC_STATUS cc_status;
HOST_INFO cc_host_info;

struct FADER {
    double grow, on, fade, off;
    double start, total;
    FADER(double g, double n, double f, double o) {
        grow = g;
        on = n;
        fade = f;
        off = o;
        start = 0;
        total = grow + on + fade + off;
    }
    bool value(const double& t, double& v) {
        if (!start) {
            start = t;
            v = 0;
            return false;
        }
        double dt = t - start;
        if (dt > total) {
            start = t;
            v = 0;
            return true;
        }
        if (dt < grow) {
            v = dt/grow;
        } else if (dt < grow+on) {
            v = 1;
        } else if (dt < grow + on + fade) {
            double x = dt-(grow+on);
            v = 1-(x/fade);
        } else {
            v = 0;
        }
        return false;
    }
};

FADER logo_fader(5,5,5,2);

/*
void show_result(RESULT* r, float x, float& y, float alpha) {
    PROGRESS_2D progress;
    char buf[256];
	grint::ttf_render_string( .1, x, y, 0, 1000., white, 0, (char*)r->project->project_name.c_str());
    y -= .02;
    float prog_pos[] = {x, y, 0};
    float prog_c[] = {.5, .4, .1, alpha/2};
    float prog_ci[] = {.1, .8, .2, alpha};
    progress.init(prog_pos, .4, -.01, -0.008, prog_c, prog_ci);
    progress.draw(r->fraction_done);
    sprintf(buf, "%.2f%% ", r->fraction_done*100);
	grint::ttf_render_string( .1, x+.41, y, 0, 1200., white, 0, buf);
    y -= .03;
    x += .05;
    sprintf(buf, "Elapsed: %.0f sec  Remaining: %.0f sec", r->elapsed_time, r->estimated_cpu_time_remaining);
	grint::ttf_render_string( .1, x, y, 0, 1200., white, 0, buf);
    y -= .03;
    sprintf(buf, "App: %s  Task: %s", (char*)r->app->user_friendly_name.c_str(),
        r->wup->name.c_str()
    );
	grint::ttf_render_string( .1, x, y, 0, 1200., white, 0, buf);
    y -= .03;
}

void show_project(unsigned int index, float alpha) {
    float x=.2, y=.6;
    char buf[1024];
	grint::ttf_render_string( .1, x, y, 0, 1200., white, 0, "This computer is participating in");
    y -= .07;
    PROJECT *p = cc_state.projects[index];
	grint::ttf_render_string( .1, x, y, 0, 500., white, 0, (char*)p->project_name.c_str());
    y -= .07;
	grint::ttf_render_string( .1, x, y, 0, 800., white, 0, (char*)p->master_url.c_str());
    y -= .05;
    sprintf(buf, "User: %s", p->user_name.c_str());
	grint::ttf_render_string( .1, x, y, 0, 800., white, 0, buf);
    y -= .05;
    if (p->team_name.size()) {
        sprintf(buf, "Team: %s",  p->team_name.c_str());
		grint::ttf_render_string( .1, x, y, 0, 800., white, 0, buf);
        y -= .05;
    }
    sprintf(buf, "Total credit: %.0f   Average credit: %.0f", p->user_total_credit, p->user_expavg_credit);
	grint::ttf_render_string( .1, x, y, 0, 800., white, 0, buf);
    y -= .05;
    if (p->suspended_via_gui) {
		grint::ttf_render_string( .1, x, y, 0, 800., white, 0, "Suspended");
    }
}

void show_disconnected() {
    float x=.3, y=.3;
	grint::ttf_render_string( .1, x, y, 0, 800., white, 0, "BOINC is not running.");
}

void show_no_projects() {
    float x=.2, y=.3;
	grint::ttf_render_string( .1, x, y, 0, 800., white, 0, "BOINC is not attached to any projects.");
    y = .25;
	grint::ttf_render_string( .1, x, y, 0, 800., white, 0, "Attach to projects using the BOINC Manager.");
}

void show_jobs(unsigned int index, double alpha) {
    float x=.1, y=.7;
    unsigned int nfound = 0;
    unsigned int i;
    cc_status.task_suspend_reason &= ~SUSPEND_REASON_CPU_USAGE_LIMIT;
    
    if (!cc_status.task_suspend_reason) {
        for (i=0; i<cc_state.results.size(); i++) {
            RESULT* r = cc_state.results[i];
            if (!r->active_task) continue;
            if (r->scheduler_state != CPU_SCHED_SCHEDULED) continue;
            if (nfound == index) {
				grint::ttf_render_string( .1f, x, y, 0, 1200.f, white, 0, "Running tasks:");
                y -= .05;
            }
            if (nfound >= index && nfound < index+4) {
                show_result(r, x, y, alpha);
                y -= .05;
            }
            nfound++;
        }
    }
    if (!nfound) {
        y = .5;
		grint::ttf_render_string( .1, x, y, 0, 500., white, 0, "No running tasks");
        char *p = 0;
        switch (cc_status.task_suspend_reason) {
        case SUSPEND_REASON_BATTERIES:
            p = "Computer is running on batteries"; break;
        case SUSPEND_REASON_USER_ACTIVE:
            p = "Computer is in use"; break;
        case SUSPEND_REASON_USER_REQ:
            p = "Computing suspended by user"; break;
        case SUSPEND_REASON_TIME_OF_DAY:
            p = "Computing suspended during this time of day"; break;
        case SUSPEND_REASON_BENCHMARKS:
            p = "Computing suspended while running benchmarks"; break;
        case SUSPEND_REASON_DISK_SIZE:
            p = "Computing suspended because no disk space"; break;
        case SUSPEND_REASON_NO_RECENT_INPUT:
            p = "Computing suspended while computer not in use"; break;
        case SUSPEND_REASON_INITIAL_DELAY:
            p = "Computing suspended while BOINC is starting up"; break;
        case SUSPEND_REASON_EXCLUSIVE_APP_RUNNING:
            p = "Computing suspended while exclusive application running"; break;
        }
        if (p) {
            y -= .1;
			grint::ttf_render_string( .1, x, y, 0, 800., white, 0, p);
        }
    }
}

*/

/*
// CMC note -- not used as we're in "unshaded mode" all the time

static void init_lights() {
   GLfloat ambient[] = {1., 1., 1., 1.0};
   GLfloat position[] = {-0.0, 3.0, 20.0, 1.0};
   GLfloat dir[] = {-1, -.5, -3, 1.0};
   glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
   glLightfv(GL_LIGHT0, GL_POSITION, position);
   glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, dir);
}

static void init_camera(double dist) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(
        45.0,       // field of view in degree
        1.0,        // aspect ratio
        1.0,        // Z near clip
        1000.0      // Z far
    );
}
*/

void app_graphics_render(int xs, int ys, double t) {
    int retval;
	double alpha;
	
#ifndef _DEMO_BUILD
    if (!connected) {
        if (t > next_connect_time) {
            retval = rpc.init("localhost");
            if (!retval) {
                retval = grint::updateData();
            }
            if (retval) {
                if (!retry_connect) {
                    exit(ERR_CONNECT);
                }
                next_connect_time = t + 10;
            } else {
                connected = true;
            }
        }
    }
	grint::bBOINCRunning = connected;
#endif	

    if (logo_fader.value(t, alpha) && connected) {
#ifndef _DEMO_BUILD
        retval = grint::updateData();
        if (retval) {
            if (!retry_connect) {
                exit(ERR_CONNECT);
            }
            connected = false;
            next_connect_time = t + 10;
			grint::bBOINCRunning = connected;
        } 
#endif
    }
	
	grint::graphics_render(xs, ys, t, alpha);
}

void app_graphics_resize(int w, int h){
	grint::graphics_resize(w, h);
}

void boinc_app_mouse_move(int x, int y, int left, int middle, int right) {}
void boinc_app_mouse_button(int x, int y, int which, int is_down) 
{
}

void boinc_app_key_press(int key1, int key2)
{	
}

void boinc_app_key_release(int, int){}

void app_graphics_init() {
	grint::graphics_init();
}

int main(int argc, char** argv) {
    int retval;
	bool test = false;
	retry_connect = true; // force test mode off & retry true by default
	
	atexit(grint::cleanup); // chance delete fonts, stop Winsock on WIN32 etc
	
    for (int i=1; i<argc; i++) {
        if (!strcmp(argv[i], "--test")) {
            test = true;
        }
        if (!strcmp(argv[i], "--retry_connect")) {
            retry_connect = true;
        }
    }

#ifdef _WIN32
    WinsockInitialize();
#endif

   if (test) {
        retval = rpc.init("localhost");
        if (!retval) {
#ifndef _DEMO_BUILD
            retval = grint::updateData();
#endif
        }
        exit(ERR_CONNECT);
    }

	grint::initVars();	

#ifdef _DEMO_BUILD
	grint::updateData(true);
#endif
	
    boinc_graphics_loop(argc, argv, "Progress Thru Processors");
    boinc_finish_diag();
#ifdef _WIN32
    WinsockCleanup();
#endif
}
