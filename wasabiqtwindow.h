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
#ifndef WASABIQTWINDOW_H
#define WASABIQTWINDOW_H
#define CURRENT_VERSION "0.7.3"

#include <QMainWindow>
#include "WASABIEngine.h"
#include <QWidget>
#include <QStandardItemModel>
#include <QTimer>
#include <QTextEdit>
#include <QXmlStreamReader>

namespace Ui {
    class WASABIQtWindow;
}

class PADWindow;
class QUdpSocket;
class WASABIqwtPlotter;

class WASABIQtWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit WASABIQtWindow(QWidget *parent = 0);
    ~WASABIQtWindow();

    bool loadInitFile(int& sPort);
    WASABIEngine* wasabi;
    bool serverAutoStart;
    bool startMinimized;
    std::string globalInitFilename;
    int getCurrentEA() {return currentEA;}
    AffectiveState* getHighlightedAffectiveState() {return highlighted_as;}
    QTimer *timer;
    QTimer *timerSender;
    bool isRunning();
    bool showXYZ();
    std::string composeEmoML(cogaEmotionalAttendee* ea);
    std::string EmoMLString;
    void setPADspace(bool state);
    void setQWT(bool state);

public slots:
    void actionAbout();
    void actionPAD_space();
    void actionPlot();

private slots:
    void on_spinBoxForceX_valueChanged(int arg1);

    void on_spinBoxForceY_valueChanged(int arg1);

    void update();

    void on_pushButtonAdd_clicked();

    void on_comboBoxEmoAttendee_currentIndexChanged(const QString &arg1);

    void on_checkBoxRun_stateChanged(int arg1);

    void on_spinBoxSlope_valueChanged(int arg1);

    void on_spinBoxMass_valueChanged(int arg1);

    void on_spinBoxUpdateRate_valueChanged(int arg1);

    void on_spinBoxAlpha_valueChanged(int arg1);

    void on_spinBoxBeta_valueChanged(int arg1);

    void on_spinBoxFactor_valueChanged(int arg1);

    void on_pushButtonMinus50_clicked();

    void on_pushButtonPlus50_clicked();

    void on_pushButtonReset_clicked();

    void on_verticalSliderP_valueChanged(int value);

    void on_horizontalSliderA_valueChanged(int value);

    void on_verticalSliderD_valueChanged(int value);

    void on_treeViewAffectiveStates_activated(const QModelIndex &index);

    void on_pushButtonTrigger_clicked();

    void processPendingDatagrams();

    void on_lineEditSenderPort_textEdited(const QString &arg1);

    void on_spinBoxSendRate_valueChanged(int arg1);

    void broadcastDatagram();

    void on_checkBoxSending_stateChanged(int arg1);

    void on_pushButton_network_send_clicked();

private:
    PADWindow *padWindow;
    WASABIqwtPlotter* qwtPlotterWindow;
    //WASABIqwtPlotter *qwtPlotterWindow;
    Ui::WASABIQtWindow *ui;
    void comboBoxAttendee_update();
    void initValues(cogaEmotionalAttendee* ea);
    QStandardItemModel *ea_model;
    void resetValues();
    void updateGUI(bool force = false);
    int currentEA;
    int guiUpdate;
    int updateRate;
    int updateRateSender;
    int addEmotionalAttendee(const QString& name, const QString& globalID);
    AffectiveState* highlighted_as;
    QUdpSocket *udpSocketReceiver;
    QUdpSocket *udpSocketSender;
    int sPort;
    // prints a message with time and status into the "Network traffic" box.
    // type: true = receive, false = transmit
    void printNetworkMessage(QString m, bool type, bool success = false, bool parsed = false);
    bool parseMessage(QString data);
    QString *networkOutputFormat;
    bool initEAbyXML(cogaEmotionalAttendee* ea);
    bool readEmotionML(QXmlStreamReader& xml, cogaEmotionalAttendee *ea);
    bool readInfo(QXmlStreamReader& xml, cogaEmotionalAttendee *ea);
    bool readEmotion(QXmlStreamReader& xml, cogaEmotionalAttendee *ea);
};

#endif // WASABIQTWINDOW_H
