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
#include "padwindow.h"
#include "glPADWidget.h"
#include "wasabiqtwindow.h"

PADWindow::PADWindow(QWidget *parent, WASABIQtWindow *window) :
    QMainWindow(parent)
{
    glPADWidget = new GLPADWidget(this, window);
    this->setCentralWidget(glPADWidget);
    this->wasabiWindow = window;

    setWindowTitle(tr("PADWindow"));
}

void PADWindow::refresh() {
    glPADWidget->repaint();
}

void PADWindow::hideEvent(QHideEvent *he) {
    wasabiWindow->setPADspace(false);//ui->actionPAD_space->setChecked(false);
}
