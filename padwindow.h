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
#ifndef PADWINDOW_H
#define PADWINDOW_H

#include <QMainWindow>

class QSlider;

class GLPADWidget;

class WASABIQtWindow;

class PADWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit PADWindow(QWidget *parent = 0, WASABIQtWindow *window = 0);

    void refresh();

protected:

    WASABIQtWindow *wasabiWindow;

    void hideEvent(QHideEvent *event);

signals:

public slots:

private:
    GLPADWidget *glPADWidget;
};

#endif // PADWINDOW_H
