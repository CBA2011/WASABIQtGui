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
#ifndef GLPADWIDGET_H
#define GLPADWIDGET_H
#include <QGLWidget>
#include "WASABIEngine.h"

class WASABIQtWindow;

class GLPADWidget : public QGLWidget
 {
    Q_OBJECT

public:
    GLPADWidget(QWidget *parent = 0, WASABIQtWindow* window = 0);
    ~GLPADWidget();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

public slots:
    void setXRotation( int angle);
    void setYRotation( int angle);
    void setZRotation( int angle);

signals:
    void xRotationChanged( int angle);
    void yRotationChanged( int angle);
    void zRotationChanged( int angle);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL( int width,  int height);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

private:
    int xRot;
    int yRot;
    int zRot;
    QPoint lastPos;
    QColor qtGreen;
    QColor qtPurple;
    WASABIQtWindow *parentWindow;
    void paintAffectiveState(const AffectiveState& as, bool highlight = false);
    WASABIEngine* wasabi;
    GLuint dynamicSpace;
    GLuint padSpace;
    GLuint padSphere;
    GLuint alphaBetaRegion, saturationRegion, activationRegion;
    GLuint makeDynamic();
    GLuint makePAD();
    GLuint makeSphere();
    GLuint makeCircle( float radius );
};
#endif // GLPADWIDGET_H
