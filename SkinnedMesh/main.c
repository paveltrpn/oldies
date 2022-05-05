/*  skinned mesh demo
 *
 *      written by Alexander Zaprjagaev
 *      frustum@public.tsu.ru
 *      2:5005/93.15@FidoNet
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <malloc.h>
#include <GL/glut.h>

#include "loadtga.h"
#include "mathlib.h"
#include "skinnedmesh.h"

skinnedmesh_t *sm;

float phi,psi,dist = 200;
float camera[4];
float fps,time;
int pause,wareframe,bone,speed = 1;

int init(void) {    
    glClearDepth(1);
    glClearColor(0,0,0,0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_LIGHT0);
    
    sm = skinnedmesh_load_ascii("data/sm.txt");
    if(!sm) return 0;
    skinnedmesh_load_skin(sm,"data/sm.skin");

    return 1;
}

/*
 *
 */

float getfps(void) {
    static float fps = 60;
    static int starttime,endtime,counter;
    if(counter == 10) {
        endtime = starttime;
        starttime = glutGet(GLUT_ELAPSED_TIME);
        fps = counter * 1000.0 / (float)(starttime - endtime);
        counter = 0;
    }
    counter++;
    return fps;
}

void _dprintf(float x,float y,char *string,...) {
    int i,l;
    char buffer[256];
    va_list argptr;
    va_start(argptr,string);
    l = vsprintf(buffer,string,argptr);
    va_end(argptr);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1,1,-1,1,-1,1);
    glRasterPos2f(x,y);
    for(i = 0; i < l; i++)
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13,buffer[i]);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

/*
 *
 */

void render_bone(skinnedmesh_t *sm) {
    int i;
    glBegin(GL_LINES);
    for(i = 0; i < sm->num_bone; i++) {
        float v0[3],v1[3];
        VectorSet(0,0,0,v0);
        VectorSet(10,0,0,v1);
        VectorTransform(v0,sm->bone[i].matrix,v0);
        VectorTransform(v1,sm->bone[i].matrix,v1);
        glVertex3fv(v0);
        glVertex3fv(v1);
    }
    glEnd();
}

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    /* set matrix */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45,4.0 / 3.0,1,2000);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(camera[0],camera[1],camera[2], 0,0,0, 0,0,1);
    glLightfv(GL_LIGHT0,GL_POSITION,camera);
    
    skinnedmesh_frame(sm,time);
    
    if(bone) {
        glColor3f(1,0,0);
        render_bone(sm);
        glColor3f(1,1,1);
    }
    
    if(wareframe == 0 || wareframe == 1) {
        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        skinnedmesh_render(sm);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
    }
    if(wareframe == 1 || wareframe == 2) {
        glColor3f(0,1,0);
        glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
        skinnedmesh_render(sm);
        glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
        glColor3f(1,1,1);
    }
    
    glColor3f(1,1,1);
    _dprintf(-0.95,0.95,"fps: %.2f time: %.2f speed: %d",fps,time,speed);
    
    glutSwapBuffers();
}

/*
 *
 */

void idle(void) {
    float ifps;
    float dir[4],up[3];
    float q0[4],q1[4],q2[4],m0[16];
    
    fps = getfps();
    ifps = 1.0 / fps;
    if(!pause) time += ifps * speed;
    
    VectorSet(0,0,1,dir);
    QuaternionSet(dir,psi,q0);
    VectorSet(0,1,0,dir);
    QuaternionSet(dir,-phi,q1);
    QuaternionMultiply(q0,q1,q2);
    QuaternionToMatrix(q2,m0);
    VectorSet(dist,0,0,camera);
    VectorTransform(camera,m0,camera);
    VectorSet(0,0,0,dir);
    VectorSet(0,0,1,up);
    VectorAdd(camera,dir,camera);
    camera[3] = 1;
    
    glutWarpPointer(400,300);
    glutPostRedisplay();
}

/*
 *
 */

void keyboard(unsigned char c,int x,int y) {
    switch(c) {
        case 27:
            exit(1);
            break;
        case ' ':
            pause = !pause;
            break;
        case 'w':
        case 'W':
            wareframe++;
            if(wareframe == 3) wareframe = 0;
            break;
        case 'b':
        case 'B':
            bone = !bone;
            break;
        case 'a':
            dist -= 10;
            if(dist < 10) dist = 10;
            break;
        case 'z':
            dist += 10;
            if(dist > 1000) dist = 1000;
            break;
        case '+':
        case '=':
            speed++;
            break;
        case '-':
            speed--;
            if(speed < 1) speed = 1;
            break;
    }
}

void mouse(int button,int state,int x,int y) {
    if(button == 3) dist -= 10;
    if(button == 4) dist += 10;
    if(dist < 30) dist = 30;
    if(dist > 1000) dist = 1000;
}

void motion(int x,int y) {
    psi += (x - 400) * 0.2;
    phi += (y - 300) * 0.2;
    if(phi < -89) phi = -89;
    if(phi > 89) phi = 89;
}

int main(int argc,char **argv) {
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutGameModeString("800x600@32");
    glutEnterGameMode();
    glutSetCursor(GLUT_CURSOR_NONE);
    if(!init()) return 1;
    glutDisplayFunc(display);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutMotionFunc(motion);
    glutPassiveMotionFunc(motion);
    glutMainLoop();
    return 0;
}
