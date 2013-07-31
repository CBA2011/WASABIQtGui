/********************************************************************************
**
** [W]ASABI [A]ffect [S]imulation [A]rchitecture for [B]elievable [I]nteractivity
**
** Copyright (C) 2011 Christian Becker-Asano.
** All rights reserved.
** Contact: Christian Becker-Asano (christian@becker-asano.de)
**
** This file is part of the WASABIQtGui program.
**
** The WASABIQtGui program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** The WASABIQtGui program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with the WASABIQtGui program.  If not, see <http://www.gnu.org/licenses/>
**
********************************************************************************/
#include <QtGui>
#include <QtOpenGL>

#include <math.h>

#include "glPADWidget.h"
#include "wasabiqtwindow.h"
#include <GL/gl.h>
#include <GL/glu.h>

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

GLPADWidget::GLPADWidget(QWidget *parent, WASABIQtWindow* window)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
    xRot = 0;
    yRot = 0;
    zRot = 0;

    qtGreen = QColor::fromCmykF(0.40, 0.0, 1.0, 0.0);
    qtPurple = QColor::fromCmykF(0.39, 0.39, 0.0, 0.0);
    if (!window) {
        std::cerr << "GLPADWidget::GLPADWidget: no window given, exiting.." << std::endl;
        exit(0);
    }
    parentWindow = window;
    wasabi = parentWindow->wasabi;
}

GLPADWidget::~GLPADWidget()
{
}

QSize GLPADWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize GLPADWidget::sizeHint() const
{
    return QSize(400, 400);
}

static void qNormalizeAngle(int &angle)
{
    while (angle < 0)
        angle += 360 * 16;
    while (angle > 360 * 16)
        angle -= 360 * 16;
}

void GLPADWidget::setXRotation( int angle)
{
    qNormalizeAngle(angle);
    if (angle != xRot)  {
        xRot = angle;
        emit xRotationChanged(angle);
        updateGL();
    }
}

void GLPADWidget::setYRotation( int angle)
{
    qNormalizeAngle(angle);
    if (angle != yRot)  {
        yRot = angle;
        emit yRotationChanged(angle);
        updateGL();
    }
}

void GLPADWidget::setZRotation( int angle)
{
    qNormalizeAngle(angle);
    if (angle != zRot)  {
        zRot = angle;
        emit zRotationChanged(angle);
        updateGL();
    }
}

void GLPADWidget::initializeGL()
{
    qglClearColor(qtPurple.dark());

    /* remove back faces */
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    /* speedups */
    glEnable(GL_DITHER);
    glShadeModel(GL_SMOOTH);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);

    static GLfloat lightPosition[4] =  { 0.5, 5.0, 7.0, 1.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    dynamicSpace = makeDynamic();
    padSpace = makePAD();
    padSphere = makeSphere();
    alphaBetaRegion = makeCircle(0.95);
    saturationRegion = makeCircle(0.95);
    activationRegion = makeCircle(0.99);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
}

void GLPADWidget::paintGL()
{

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glFrustum( -100.0, 100.0, -100.0, 100.0, 500.0, 5000.0 );
    // Checking whether or not to paint the 3D dynamic space:
    glTranslatef(0,0,-1000);
    glRotatef(xRot / 16.0, 1.0, 0.0, 0.0);
    glRotatef(yRot / 16.0, 0.0, 1.0, 0.0);
    glRotatef(zRot / 16.0, 0.0, 0.0, 1.0);
    glMatrixMode( GL_MODELVIEW );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    if (parentWindow && !parentWindow->showXYZ()) {
        glPushMatrix();
        glCallList( padSpace );
        QGLWidget::renderText(120,0,0,"+A");
        QGLWidget::renderText(0,120,0,"+P");
        QGLWidget::renderText(0,0,120,"+D");
        glPopMatrix();
    }



    //now we draw the affective states
    glPushMatrix();
    //PADdots NEW
    int currentEA = 1;
    currentEA = parentWindow->getCurrentEA();

    if (cogaEmotionalAttendee* ea = wasabi->getEAfromID(currentEA)) {
        if (parentWindow && !parentWindow->showXYZ()) {
            std::vector<AffectiveState*>::const_iterator iter_as =
                    ea->EmoConPerson->affectiveStates.begin();
            for (; iter_as != ea->EmoConPerson->affectiveStates.end(); ++iter_as) {
                if (parentWindow && parentWindow->getHighlightedAffectiveState() == *iter_as) {
                    paintAffectiveState((**iter_as), true);
                } else {
                    paintAffectiveState((**iter_as));
                }
            }
        }
        glPopMatrix();

        // Checking whether or not to paint the 3D dynamic space:
        if (ea->EmoConPerson->xReg != 0 && ea->EmoConPerson->yReg != 0 && parentWindow
                && parentWindow->showXYZ()) {
            glPushMatrix();
            GLfloat color[4];
            glGetFloatv(GL_CURRENT_COLOR, color);
            glColor3f(1.0, 1.0, 1.0);

            glCallList(dynamicSpace);
            glColor3f(.8, .8, 1.0);

            std::vector<cogaEmotionalAttendee*>::iterator iter_ea;
            for (iter_ea = wasabi->emoAttendees.begin(); iter_ea
                 != wasabi->emoAttendees.end(); ++iter_ea) {
                cogaEmotionalAttendee* ea = (*iter_ea);
                glPushMatrix();
                glTranslatef((float) ea->getXPos(), (float) ea->getYPos(),
                             (float) ea->getZPos());
                glCallList(padSphere); // This will paint the sphere in blue for dynamic space

                glTranslatef(0, -10, 0);
                glRotatef(180, 1.0f, 0.0f, 0.0f);
                QGLWidget::renderText(0,0,0,ea->getName().c_str());

                glPopMatrix();
            }

            glPushMatrix();
            QGLWidget::renderText(120,0,0,"X");
            QGLWidget::renderText(0,120,0,"Y");
            QGLWidget::renderText(0,0,120,"Z");
            glPopMatrix();
            glScalef((float) ea->EmoConPerson->xReg,
                     (float) ea->EmoConPerson->yReg, 1);
            glCallList(alphaBetaRegion);
            glColor4fv(color);
            glPopMatrix();
        }
    }
    if (parentWindow && !parentWindow->showXYZ()) {
        //padSphere for the agent
        GLfloat color[4];
        glGetFloatv(GL_CURRENT_COLOR, color);
        std::vector<cogaEmotionalAttendee*>::iterator iter_ea;
        for (iter_ea = wasabi->emoAttendees.begin(); iter_ea
             != wasabi->emoAttendees.end(); ++iter_ea) {
            cogaEmotionalAttendee* ea = (*iter_ea);
            glPushMatrix();
            glTranslatef((float) ea->getAValue(), (float) ea->getPValue(),(float) ea->getDValue());
            glColor3f(0.8, 0.8, 0.8);
            glCallList(padSphere);
            glTranslatef(0, -10, 0);
            QGLWidget::renderText(0,0,0,ea->getName().c_str());
            glPopMatrix();
        }
        glColor4fv(color);
    }
    glFlush();

    //QGLWidget::swapBuffers();
}

void GLPADWidget::resizeGL( int width, int height)
{
    int side = qMin(width, height);
    glViewport((width - side) / 2, (height - side) / 2, side, side);

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glFrustum( -10.0, 10.0, -10.0, 10.0, 50.0, 500.0 );
    glTranslatef(0,0,-200);
    glMatrixMode( GL_MODELVIEW );
}

void GLPADWidget::mousePressEvent(QMouseEvent *event)
{
    lastPos = event->pos();
}

void GLPADWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - lastPos.x();
    int dy = event->y() - lastPos.y();

    if (event->buttons() & Qt::LeftButton)  {
        setXRotation(xRot + 8 * dy);
        setYRotation(yRot + 8 * dx);
    } else if (event->buttons() & Qt::RightButton)  {
        setXRotation(xRot + 8 * dy);
        setZRotation(zRot + 8 * dx);
    }
    lastPos = event->pos();
}

void GLPADWidget::paintAffectiveState(const AffectiveState& as, bool highlight) {
    std::vector<AffectPolygon*> polygons = as.polygons;
    double as_intens = as.getIntensity();
    std::vector<AffectPolygon*>::iterator iter_ap;
    for (iter_ap = polygons.begin(); iter_ap != polygons.end(); ++iter_ap) {
        std::vector<AffectVertex*> verts = (*iter_ap)->vertices;
        // Do we have a single point or a complete vertex here?
        glLoadIdentity();
        if ((*iter_ap)->glMode == "QUAD" && verts.size() == 4 && (as_intens
                                                                  > 0.1 || highlight)) {
            std::vector<AffectVertex*>::iterator iter_av = verts.begin();
            GLuint drawmode = GL_QUADS;
            if (as_intens < 0.5) {
                drawmode = GL_LINE_LOOP;
            }
            glBegin(drawmode);
            for (; iter_av != verts.end(); iter_av++) {
                AffectVertex* vertex = (*iter_av);
                GLfloat vec[3] = { ((float) vertex->coords[1]),
                                   ((float) vertex->coords[0]),
                                   ((float) vertex->coords[2]) };
                float intens = vertex->intensity * as_intens;
                glColor3f(0.1, 0.1, 0.1);
                if (highlight) {
                    intens = vertex->intensity; // we want to see the base intensity!
                    glColor3f(intens, intens, 1.0);
                } else {
                    glColor3f(intens, intens, intens);
                }
                glVertex3fv(vec);
            }
            glEnd();
        } else if ((*iter_ap)->glMode == "GL_POINTS" && verts.size() == 1) { // i.e. a point
            AffectVertex* vertex = verts.at(0); //verts[0];
            GLfloat vec[3] = { (GLfloat) vertex->coords[1],
                               (GLfloat) vertex->coords[0], (GLfloat) vertex->coords[2] };
            if (vertex->likelihood > 0.3) {
                glPointSize(vertex->likelihood * 10);
            } else {
                glPointSize(3);
            }
            if (highlight) {
                glTranslatef(vertex->coords[1], vertex->coords[0],
                             vertex->coords[2]);
                glColor3f(0.5, 0.5, 1.0);
                glCallList(saturationRegion);
                glLoadIdentity();
            }
            glBegin(GL_POINTS);
            glColor3f(0.1, 0.1, 1.0);
            if (as_intens > 0.1) {
                glColor3f(as_intens, as_intens, 1.0);
            }
            glVertex3fv(vec);
            glEnd();

            GLfloat color[4];
            glGetFloatv(GL_CURRENT_COLOR, color);
            glLoadIdentity();
            glTranslatef(vertex->coords[1], vertex->coords[0],
                         vertex->coords[2]);
            float factor = ((float) (*iter_ap)->min_distance) * 100;
            glScalef(factor, factor, factor);
            glColor3f(0.5, 0.2, 0.2);
            glCallList(activationRegion);
            glScalef(1 / factor, 1 / factor, 1 / factor);
            factor = (float) ((*iter_ap)->max_distance) * 100;
            glScalef(factor, factor, factor);
            glColor3f(1.0, 0.4, 0.4);
            glCallList(saturationRegion);
            glScalef(1 / factor, 1 / factor, 1 / factor);
            glColor4fv(color);
        } // end verts.size() == 1
    }
}

GLuint GLPADWidget::makeDynamic()
{
    GLuint list;
    list = glGenLists( 1 );

    glNewList( list, GL_COMPILE );

    glLineWidth( 3.0 );

    glBegin( GL_LINE_LOOP );
    glVertex3f(  100.0,  100.0, 0.0 );
    glVertex3f(  100.0, -100.0, 0.0 );
    glVertex3f( -100.0, -100.0, 0.0 );
    glVertex3f( -100.0,  100.0, 0.0 );
    glEnd();

    glLineWidth( 1.0 );

    glBegin( GL_LINE_STRIP );
    glVertex3f(  0.0,  -100.0, 0.0 );
    glVertex3f(  0.0, 100.0, 0.0 );
    glVertex3f( -10.0, 90.0, 0.0 );
    glVertex3f( 10.0,  90.0, 0.0 );
    glVertex3f( 0.0,  100.0, 0.0 );
    glEnd();

    glBegin( GL_LINE_STRIP );
    glVertex3f(  -100.0,  0.0, 0.0 );
    glVertex3f(  100.0, 0.0, 0.0 );
    glVertex3f( 85.0, -5.0, 0.0 );
    glVertex3f( 85.0,  5.0, 0.0 );
    glVertex3f(  100.0, 0.0, 0.0 );
    glEnd();

    glBegin( GL_LINE_STRIP );
    glVertex3f(  0.0,  0.0, 0.0 );
    glVertex3f(  0.0, 0.0, -100.0 );
    glVertex3f( 0.0, -5.0, -95.0 );
    glVertex3f( 0.0,  5.0, -95.0 );
    glVertex3f(  0.0, 0.0, -100.0 );
    glVertex3f( -5.0, 0.0, -95.0 );
    glVertex3f( 5.0,  0.0, -95.0 );
    glVertex3f(  0.0, 0.0, -100.0 );
    glEnd();

    glEndList();

    return list;
}

/*!
  Make a display list for PAD space rendering
*/
GLuint GLPADWidget::makePAD()
{
    GLuint list;

    list = glGenLists( 1 );

    glNewList( list, GL_COMPILE );

    glColor3f(0.7, 0.7, 0.7);

    glLineWidth( 2.0 );

    glBegin( GL_LINE_LOOP );
    glVertex3f(  100.0,  100.0, -100.0 );
    glVertex3f(  100.0, -100.0, -100.0 );
    glVertex3f( -100.0, -100.0, -100.0 );
    glVertex3f( -100.0,  100.0, -100.0 );
    glEnd();

    glBegin( GL_LINE_LOOP );
    glVertex3f(  100.0,  100.0, 100.0 );
    glVertex3f(  100.0, -100.0, 100.0 );
    glVertex3f( -100.0, -100.0, 100.0 );
    glVertex3f( -100.0,  100.0, 100.0 );
    glEnd();

    glBegin( GL_LINES );
    glVertex3f(  100.0,  100.0, -100.0 );   glVertex3f(  100.0,  100.0, 100.0 );
    glVertex3f(  100.0, -100.0, -100.0 );   glVertex3f(  100.0, -100.0, 100.0 );
    glVertex3f( -100.0, -100.0, -100.0 );   glVertex3f( -100.0, -100.0, 100.0 );
    glVertex3f( -100.0,  100.0, -100.0 );   glVertex3f( -100.0,  100.0, 100.0 );
    glEnd();

    glBegin (GL_LINE_LOOP);
    glVertex3f( -120.0,  0.0,  0.0 );   glVertex3f(  120.0,  0.0, 0.0 );
    glVertex3f( 105.0,  -5.0,  0.0 );   glVertex3f(  105.0,  5.0, 0.0 );  glVertex3f(  120.0,  0.0, 0.0 );
    glEnd();

    glBegin (GL_LINE_LOOP);
    //glVertex3f( -120.0,  0.0,  0.0 );   glVertex3f(  120.0,  0.0, 0.0 );
    glVertex3f(  0.0, -120.0,  0.0 );   glVertex3f(  0.0,  120.0, 0.0 );
    glVertex3f(  -10.0, 110.0,  0.0 );   glVertex3f(  10.0,  110.0, 0.0 );  glVertex3f( 0.0,  120.0, 0.0 );
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(  0.0,  0.0, -120.0 );   glVertex3f(  0.0,  0.0, 120.0 );
    glVertex3f(  0.0,  -5.0, 110.0 );   glVertex3f(  0.0,  5.0, 110.0 );  glVertex3f(  0.0,  0.0, 120.0 );
    glEnd();

    glEndList();

    return list;
}

/*!
  Generating a "display list" for the sphere (including texture coordinates)
*/
GLuint GLPADWidget::makeSphere() // const QImage& tex)
{
    glLoadIdentity();
    GLUquadricObj* q = gluNewQuadric();
    GLuint obj = glGenLists(1);
    glNewList( obj, GL_COMPILE );
    glRotatef( -90, 1.0, 0.0, 0.0 );

    gluSphere(q, 5.0, 12, 12);
    glEndList();
    gluDeleteQuadric( q );
    return obj;
}

/*!
  Generating the 2D-circles with initial radius
*/
GLuint GLPADWidget::makeCircle( float radius)
{
    glLoadIdentity();
    GLUquadricObj* q = gluNewQuadric();
    GLuint obj = glGenLists(1);
    glNewList( obj, GL_COMPILE );

    gluQuadricTexture( q, GL_TRUE );
    gluDisk(q, radius, 1.01, 24, 2);
    glEndList();
    gluDeleteQuadric( q );
    return obj;
}

