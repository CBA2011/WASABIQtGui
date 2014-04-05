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
#include "wasabiqtwindow.h"
#include "ui_wasabiqtwindow.h"
#include "padwindow.h"
#include "wasabiqwtplotter.h"
#include "PrimaryEmotion.h"
#include "SecondaryEmotion.h"
#include <sstream>
#include <fstream>
#include <string>
#include <QDebug>
#include <QDir>
#include <QInputDialog>
#include <QtGui>
#include <QtNetwork>
#include <QMessageBox>
#include <QErrorMessage>

// some helpers from http://www.gammon.com.au/forum/bbshowpost.php?bbsubject_id=2896

#define SPACES " \t\r\n"
using namespace std;
inline string trim_right(const string & s, const string & t = SPACES) {
    string d(s);
    string::size_type i(d.find_last_not_of(t));
    if (i == string::npos)
        return "";
    else
        return d.erase(d.find_last_not_of(t) + 1);
} // end of trim_right

inline string trim_left(const string & s, const string & t = SPACES) {
    string d(s);
    return d.erase(0, s.find_first_not_of(t));
} // end of trim_left

inline string trim(const string & s, const string & t = SPACES) {
    string d(s);
    return trim_left(trim_right(d, t), t);
} // end of trim

void myReplace(std::string& str, const std::string& oldStr, const std::string& newStr)
{
    size_t pos = 0;
    while((pos = str.find(oldStr, pos)) != std::string::npos)
    {
        str.replace(pos, oldStr.length(), newStr);
        pos += newStr.length();
    }
}

// Used internally to temporarily store data collected from the xml file
struct emodata {
    std::string category; // fsre-, occ-, or everyday-category name, e.g. 'relaxed'
    std::string emoclass; // primary or secondary
    int xTens;
    int yTens;
    int slope;
    int mass;
    int xReg;
    int boredom;
    int prevalence;
    float base_intensity;
    float act_threshold;
    float sat_threshold;
    std::string decay;
    float pleasure, arousal, dominance;
    float lifetime;
    std::vector<AffectPolygon*> affect_polygons;
} tmp_emodata;

WASABIQtWindow::WASABIQtWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::WASABIQtWindow)
{
    ui->setupUi(this);
    //setCentralWidget(ui->tabWidgetMain); // DON'T EVER DO THIS! Otherwise other parts are invisible!!!
    globalInitFilename = "WASABI.ini";
    wasabi = new WASABIEngine("secondary");
    wasabi->setMaxSimulations(1000);
    currentEA = 1;
    guiUpdate = 0;
    updateRate = 50;
    updateRateSender = 1;
    lastPackageTimeStamp = 0;
    serverAutoStart = false;
    startMinimized = false;
    networkOutputFormat = new QString("default");
    QDir dir;
    qDebug() << "Hello World " << dir.currentPath();
    sPort = 42424;

    // Create the qwt-based plotter window
    qwtPlotterWindow = new WASABIqwtPlotter(this, wasabi);
    qwtPlotterWindow->resize(qwtPlotterWindow->sizeHint());
    // The plotter is under construction
    //qwtPlotterWindow->show();

    // Create the pad space OpenGL-window
    padWindow = new PADWindow(this, this);
    padWindow->resize(padWindow->sizeHint());
    padWindow->show();

    if (!loadInitFile(sPort)) {
        qDebug() << "WASABIQtWindow::WASABIQtWindow: unable to load WASABI.ini or something went wrong!";
        QErrorMessage *error = new QErrorMessage(this);
        QString errorMessage;
        errorMessage.append("Unable to load WASABI.ini, which should be in ").append(dir.currentPath()).append("!");
        error->showMessage(errorMessage);
    }
    qDebug() << "WASABIQtWindow::WASABIQtWindow: serverPort is '" << sPort << "'";

    // create the main gui window
    /* If WASABI.ini for a given EA does not reference a *.emo_dyn AND a *.emo_pad file, but only a *.xml file,
     * then initAllEAs() will fail for that EA and ea->initialized will remain false.
     * To keep Qt out of the WASABIEngine library, we need to check here all uninitialized ea's for their respective
     * ea->xmlFilename(s) and parse these using Qt functionality.
     */
    wasabi->initAllEAs();
    std::vector<cogaEmotionalAttendee*>::iterator iter_ea;
    for (iter_ea = wasabi->emoAttendees.begin(); iter_ea != wasabi->emoAttendees.end(); ++iter_ea){
        cogaEmotionalAttendee* ea = (*iter_ea);
        if (!(ea->initialized)){
            qDebug() << "WASABIQtWindow::Trying to initialize EA " << ea->getLocalID() << " with xml file " << ea->EmoConPerson->xmlFilename.c_str();
            if (initEAbyXML(ea)) {
                std::cout << "WASABIQtWindow: xml initialization successful!" << std::endl;
            }
            else {
                std::cerr << "WASABIQtWindow: xml initialization FAILED!!!" << std::endl;
                QErrorMessage *error = new QErrorMessage(this);
                QString errorMessage;
                errorMessage.append("Failure to load ").append(ea->EmoConPerson->xmlFilename.c_str()).append(", (which should be in ").append(dir.currentPath()).append("/xml/)!");
                error->showMessage(errorMessage);
            }
        }
    }

    comboBoxAttendee_update();
    initValues(wasabi->getEAfromID());

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(1000/updateRate); // 50Hz

    timerSender = new QTimer(this);
    connect(timerSender, SIGNAL(timeout()), this, SLOT(broadcastDatagram()));
    timerSender->start(1000/updateRateSender); // 50Hz

    ////// Server stuff from network/broadcastreceiver/receiver.cpp
    udpSocketReceiver = new QUdpSocket(this);
    if (udpSocketReceiver->bind(sPort+1, QUdpSocket::ShareAddress)) {//
        std::cout << "The udpSocket receiving on port: " << sPort+1 << std::endl;

        connect(udpSocketReceiver, SIGNAL(readyRead()),
                this, SLOT(processPendingDatagrams()));

        udpSocketSender = new QUdpSocket(this);
    }
    else {
        QMessageBox::critical(this, tr("Threaded WASABI Server"),
                              tr("Unable to start the server"));
    }


    QString str;
    str.setNum(sPort);
    ui->lineEditSenderPort->setText(str); // udpSocketSender must have been initialized first!
    str.setNum(sPort+1);
    ui->lineEditReceiverPort->setText(str);
    connect(ui->actionAbout, SIGNAL(triggered()), SLOT(actionAbout()) );
    connect(ui->actionPAD_space, SIGNAL(triggered()), SLOT(actionPAD_space()) );
    connect(ui->actionPlot, SIGNAL(triggered()), SLOT(actionPlot()) );
}

WASABIQtWindow::~WASABIQtWindow()
{
    delete ui;
}

void WASABIQtWindow::update() {
    if (!wasabi) {
        qDebug() << "WASABIQtWindow::update(): no wasabi engine set!";
        return;
    }
    wasabi->update();
    if (padWindow) {
        padWindow->refresh();
    }
    else {
        qDebug() << "WASABIQtWindow::update(): no padWindow, no refresh..";
    }

    updateGUI();
}

void WASABIQtWindow::updateGUI(bool force) {
    cogaEmotionalAttendee* ea = wasabi->getEAfromID(currentEA);
    if (!ea || !ui){
        return;
    }
    QString tmp = QString("(%0, %1, %2)").arg(ea->getPValue()).arg(ea->getAValue()).arg(ea->getDValue());
    ui->textEditPAD->setPlainText(tmp);

    tmp = QString("(%0, %1, %2)").arg(ea->getXPos()).arg(ea->getYPos()).arg(ea->getZPos());
    ui->textEditXYZ->setPlainText(tmp);
    //temp.str("");
    guiUpdate += updateRate;
    if (guiUpdate > 499 || force) {
        guiUpdate = 0;
        // just a test with uid = 0
        std::string padString;
        if (wasabi->getPADString(padString, currentEA)) {
            myReplace(padString, "&", " ");
        } else {
            std::cout << "WASABIQtWindow::updateGUI: No padString found!" << std::endl;
        }
    }
    if (padWindow) {
        padWindow->refresh();
    }
}

bool WASABIQtWindow::loadInitFile(int& sPort) {
    qDebug() << "WASABIQtWindow::loadInitFile(): Trying to find and parse '" << globalInitFilename.c_str() << "'!";
    //Open file for reading
    std::ifstream file(globalInitFilename.c_str());
    std::string line;
    if (file) {
        std::string word1;
        std::string word2;
        int newLocalID;
        int number;
        cogaEmotionalAttendee* ea;
        while (std::getline(file, line)) {
            line = trim(line);
            if (!line.empty() && (line.at(0)) != '#') { //comments begin with '#'
                qDebug() << "> " << line.c_str();
                std::istringstream isline(line);
                isline >> word1;
                //cout << line << endl;
                if (!(word1.empty())) {
                    //cout << "(" << word1 << ")" << endl;
                    isline >> word2;
                    //cout << "-->(" << word2 << ")" << endl;
                    if (!(word2.empty())) {
                        switch (returnIndex(word1, "EA ServerAutoStart ServerPort StartMinimized MaxSimulations")) {
                        case 1: //EA: This starts a block of data for a specific EmotionalAttendee. Has to be terminated by \EA
                            // addEmotionalAttendee return 0 only in case of failure to create the new EmotionalAttendee
                            if (newLocalID = wasabi->addEmotionalAttendee(word2)) {
                                qDebug() << "newLocalID assigned to " << QString(word2.c_str()) << " is " << newLocalID;
                                // The EA was just initialized with "init.[dyn|pad]" files automatically
                                bool initByXML = false; // We set this to true, when an 'xmlFilename' key was found.
                                while (std::getline(file, line) && trim(line) != "EA_END") { // We stop if a line starts with EA_END, see above
                                    line = trim(line);
                                    if (!line.empty() && (line.at(0)) != '#') { //comments begin with '#'
                                        qDebug() << "> " << line.c_str();
                                        std::istringstream isline(line);
                                        isline >> word1;
                                        if (!(word1.empty())) {
                                            isline >> word2;
                                            ea = wasabi->getEAfromID(newLocalID);
                                            if (!(word2.empty())) {
                                                switch (returnIndex(word1, "dynFilename padFilename globalID simulationOn xmlFilename")) {
                                                case 1: // dynFilename
                                                    if (ea && !initByXML ) {
                                                        ea->EmoConPerson->dynFilename = word2;
                                                    } else {
                                                        qDebug()
                                                                << "MyApp::loadInitFile(): ERROR no ea with ID "
                                                                << newLocalID
                                                                << " found!"
                                                                   ;
                                                    }
                                                    break;
                                                case 2: // padFilename
                                                    if (ea && !initByXML) {
                                                        ea->EmoConPerson->padFilename = word2;
                                                    } else {
                                                        qDebug()
                                                                << "MyApp::loadInitFile(): ERROR no ea with ID "
                                                                << newLocalID
                                                                << " found!"
                                                                   ;
                                                    }
                                                    break;
                                                case 3: //globalID
                                                    if (ea) {
                                                        ea->setGlobalID(word2);
                                                    } else {
                                                        qDebug()
                                                                << "MyApp::loadInitFile(): ERROR no ea with ID "
                                                                << newLocalID
                                                                << " found!"
                                                                   ;
                                                    }
                                                    break;
                                                case 4: // simulationOn
                                                    if (ea) {
                                                        if (word2 == "false" || word2 == "0") {
                                                            ea->simulationOn = false; // default value is true, see cogaEmotionalAttendee
                                                        }
                                                    }
                                                    break;
                                                case 5: // xmlFilename
                                                    if (ea) {
                                                        ea->EmoConPerson->xmlFilename = word2;
                                                        initByXML = true;
                                                    } else {
                                                        qDebug()
                                                                << "MyApp::loadInitFile(): ERROR no ea with ID "
                                                                << newLocalID
                                                                << " found!"
                                                                   ;
                                                    }
                                                    break;
                                                default:
                                                    if (word1 != "EA_END"){
                                                        qDebug()
                                                                << "MyApp::loadInitFile: keyword '"
                                                                << word1.c_str()
                                                                << "' not recognized in EA context!"
                                                                   ;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            break;
                        case 2: //ServerAutoStart
                            serverAutoStart = false;
                            if (word2 == "true" || word2 == "1") {
                                serverAutoStart = true; // false is default value
                            }
                            break;
                        case 3: //ServerPort
                            number = atoi(word2.c_str());
                            if (number > 500 && number < 65536) {
                                sPort = number; // default is 42424
                            }
                            break;
                        case 4: //StartMinimized
                            startMinimized = false;
                            if (word2 == "true" || word2 == "1") {
                                startMinimized = true; // false is default value
                            }
                            break;
                        case 5: //MaxSimulations
                            // TODO: Fix this thing!!!
                            //MaxSimulations = 10;
                            number = atoi(word2.c_str());
                            if (number > 0 && number < 50) {
                                //MaxSimulations = number;
                            }
                            break;
                        default:
                            qDebug()
                                    << "MyApp::loadInitFile: Error: Unknown keyword \""
                                    << word1.c_str() << "\"!";
                        }
                    }
                }
            } // end if ((line.at(0)) != '#')
        }//end while
        file.close();
    } //if (file)
    else {
        qDebug()
                << "MyApp::loadInitFile: ERROR Could not open file! --> creating dummy JohnDoe :)"
                   ;
        wasabi->addEmotionalAttendee("JohnDoe");
        return false; //could not open file;
    }
    return true;
}


void WASABIQtWindow::resetValues()
{
    if (!wasabi) {
        std::cout << "myGuiFrame::resetValues(): ERROR no wasabi engine found.." << std::endl;
        return;
    }
    cogaEmotionalAttendee* ea = wasabi->getEAfromID(currentEA);
    if (!ea){
        std::cerr << "myGuiFrame::resetValues(): ERROR no cogaEmotionalAttendee found.." << std::endl;
        return;
    }
    ea->setXTens(ui->spinBoxForceX->value());
    ea->setYTens(ui->spinBoxForceY->value());
    ea->setSlope(ui->spinBoxSlope->value());
    ea->setMass(ui->spinBoxMass->value());
    ea->setAlpha(ui->spinBoxAlpha->value());
    ea->setBeta(ui->spinBoxBeta->value());
    ea->setFactor(ui->spinBoxFactor->value());
    ea->setUpdateRate(ui->spinBoxUpdateRate->value());
}

void WASABIQtWindow::initValues(cogaEmotionalAttendee* ea)
{
    if (ea) {
        ui->spinBoxForceX->setValue(ea->getXTens());
        ui->spinBoxForceY->setValue(ea->getYTens());
        ui->spinBoxSlope->setValue(ea->getSlope());
        ui->spinBoxMass->setValue(ea->getMass());
        //ui->spinBoxUpdateRate->setValue(ea->getUpdateRate());
        ui->spinBoxAlpha->setValue(ea->getAlpha());
        ui->spinBoxBeta->setValue(ea->getBeta());
        ui->spinBoxFactor->setValue(ea->getFactor());
        ui->treeViewAffectiveStates->selectAll();
        ui->treeViewAffectiveStates->clearSelection();
        ea_model = new QStandardItemModel();
        QStandardItem *parentItem = ea_model->invisibleRootItem();
        QStandardItem *item = new QStandardItem(QString(ea->getName().c_str()));
        parentItem->appendRow(item);

        std::vector<AffectiveState*>::iterator iter_as;
        bool peItemCreated = false;
        bool seItemCreated = false;
        QStandardItem *primaryItem;
        QStandardItem *secondaryItem;
        for (iter_as = ea->EmoConPerson->affectiveStates.begin(); iter_as!=ea->EmoConPerson->affectiveStates.end(); iter_as++) {
            AffectiveState* as = *iter_as;

            PrimaryEmotion* pe;
            pe = dynamic_cast<PrimaryEmotion*>(as);
            SecondaryEmotion* se;
            se = dynamic_cast<SecondaryEmotion*>(as);
            QStandardItem *new_item;

            if (pe) {
                if (!peItemCreated) {
                    primaryItem = new QStandardItem(QString("Primary"));
                    item->appendRow(primaryItem);
                    peItemCreated = true;
                }
                new_item = new QStandardItem(QString(pe->type.c_str()));
                primaryItem->appendRow(new_item);
            }
            else if (se) {
                if (!seItemCreated) {
                    secondaryItem = new QStandardItem(QString("Secondary"));
                    item->appendRow(secondaryItem);
                    seItemCreated = true;
                }
                new_item = new QStandardItem(QString(se->type.c_str()));
                secondaryItem->appendRow(new_item);
            }
            if (new_item) {
                std::vector<AffectPolygon*>::iterator iter_poly;
                int polycount = 1;
                for (iter_poly = as->polygons.begin(); iter_poly!=as->polygons.end(); iter_poly++) {
                    AffectPolygon* p = *iter_poly;
                    QStandardItem *poly = new QStandardItem(QString("polygon#%0").arg(polycount));
                    new_item->appendRow(poly);
                    int vertexcount = 1;
                    std::vector<AffectVertex*>::iterator iter_av;
                    for (iter_av = p->vertices.begin(); iter_av!=p->vertices.end(); iter_av++) {
                        AffectVertex* av = *iter_av;
                        QStandardItem *vertex = new QStandardItem(QString("vertex#%0 (%1,%2,%3)").arg(vertexcount).arg(av->coords[0]).arg(av->coords[1]).arg(av->coords[2]));
                        poly->appendRow(vertex);
                        vertexcount++;
                    }
                    polycount++;
                }
            }
            else {
                std::cout << "myGuiFrame::updateValues: affect pointer == NULL!" << std::endl;
            }
        }
        ui->treeViewAffectiveStates->setModel(ea_model);
        std::cout << "myGuiFrame::updateValues: finished!" << std::endl;
    }
    else {
        std::cerr << "myGuiFrame::updateValues(): ERROR no EmotionalAttendee given.." << std::endl;
    }
}

void WASABIQtWindow::comboBoxAttendee_update()
{
    if (!wasabi){
        return;
    }
    QString selection = ui->comboBoxEmoAttendee->currentText();
    ui->comboBoxEmoAttendee->clear();
    std::vector<cogaEmotionalAttendee*>::iterator iter_ea;
    for (iter_ea = wasabi->emoAttendees.begin(); iter_ea != wasabi->emoAttendees.end(); ++iter_ea){
        cogaEmotionalAttendee* ea = (*iter_ea);
        QString item;
        std::ostringstream temp;
        temp << ea->getName().c_str() << "/" << ea->getLocalID();
        item.append(temp.str().c_str());
        ui->comboBoxEmoAttendee->addItem(item);
    }
    if (ui->comboBoxEmoAttendee->findText(selection) == -1) {
        if (ui->comboBoxEmoAttendee->count() > 0) {
            ui->comboBoxEmoAttendee->setCurrentIndex(0);
        }
    }
    else {
        ui->comboBoxEmoAttendee->setCurrentIndex(ui->comboBoxEmoAttendee->findText(selection));
    }
}

bool WASABIQtWindow::isRunning() {
    return ui->checkBoxRun->isChecked();
}

bool WASABIQtWindow::showXYZ() {
    return ui->checkBoxShowIn3D->isChecked();
}

/** Checks if supplied globalID already in use.
 * If not, uses the wasabi engine to create a new EmotionalAttendee
 * for which a new localID is generated and returned.
 */
int WASABIQtWindow::addEmotionalAttendee(const QString& name, const QString& globalID) {
    // we might wanna perform some checks here in the future, before adding the EmotionalAttendee
    //long int globalID_int = -1; //i.e. undef
    bool converted;
    int globalID_int = (int)globalID.toLong(&converted);
    if (converted && wasabi->getEAfromID(globalID_int)) {
        std::cout << "myGuiFrame::addEmotionalAttendee: ERROR globalID " << globalID.toStdString() << " already in use!" << std::endl;
        return 0; // means ERROR here
    }

    return wasabi->addEmotionalAttendee(name.toStdString(), globalID.toStdString());

    /*int newLocalID = wasabi->addEmotionalAttendee(name.toStdString(), globalID.toStdString());
    if (newLocalID != 0 && wasabi->initEA(wasabi->getEAfromID(newLocalID))){
        comboBoxAttendee_update();

        currentEA = newLocalID;
        cogaEmotionalAttendee* ea = wasabi->getEAfromID(currentEA);
        if (ea) {
            initValues(ea);
            ui->statusBar->showMessage(QString("Attendee %0 initialized").arg(ea->getName().c_str()), 1000);
        }

        return newLocalID;
    }
    std::cout << "myGuiFrame::addEmotionalAttendee: ERROR could not create new EA or it could not be initialized!" << std::endl;
    return newLocalID;*/
}

void WASABIQtWindow::updateGuiAfterAddingAgent(cogaEmotionalAttendee* ea){
    comboBoxAttendee_update();
    initValues(ea);
    ui->statusBar->showMessage(QString("Attendee %0 initialized").arg(ea->getName().c_str()), 1000);
}

void WASABIQtWindow::on_spinBoxForceX_valueChanged(int arg1)
{
    if (arg1 >= 0 && wasabi) {
        wasabi->setXForce(arg1, currentEA);
    }
}

void WASABIQtWindow::on_spinBoxForceY_valueChanged(int arg1)
{
    if (arg1 >= 0 && wasabi) {
        wasabi->setYForce(arg1, currentEA);
    }
}

void WASABIQtWindow::on_pushButtonAdd_clicked()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("WASABIQtGui: Please provide input.."),
                                         tr("Name of new Attendee:"), QLineEdit::Normal,
                                         "Geminoid F", &ok);
    if (!ok || name.isEmpty()) {
        return;
    }
    name = name.replace('/', '_');//because '/' is used to seperate name from id

    QString globalID = QInputDialog::getText(this, tr("WASABIQtGui: Please provide input.."),
                                             tr("GlobalID of new Attendee:"), QLineEdit::Normal,
                                             "gf1", &ok);
    if (!ok || globalID.isEmpty()) {
        return;
    }
    addEmotionalAttendee(name, globalID);
}

void WASABIQtWindow::on_comboBoxEmoAttendee_currentIndexChanged(const QString &arg1)
{
    QStringList str_list = arg1.split("/");
    if (str_list.size() != 2) {
        std::cerr << "WASABIQtWindow::on_comboBoxEmoAttendee_currentIndexChanged: str_list.size() != 2, bailing out.." << std::endl;
        return;
    }
    bool ok;
    currentEA = str_list.at(1).toLong(&ok); //Note, we ignore the name at str_list.at(0)!!!
    if (ok && wasabi) {
        cogaEmotionalAttendee* ea = wasabi->getEAfromID(currentEA);
        if (ea) {
            initValues(ea);
            ui->statusBar->showMessage(QString("Attendee %0 initialized").arg(ea->getName().c_str()), 1000);
        }
    }
}

void WASABIQtWindow::on_checkBoxRun_stateChanged(int arg1)
{
    switch (arg1) {
    case 0: //unchecked
        timer->stop();
        break;
    case 2: // fully checked (instead of partially checked state 1)
        timer->start();
    }

}

void WASABIQtWindow::on_spinBoxSlope_valueChanged(int arg1)
{
    if (arg1 >= 0 && wasabi) {
        wasabi->setSlope(arg1, currentEA);
    }
}

void WASABIQtWindow::on_spinBoxMass_valueChanged(int arg1)
{
    if (arg1 >= 0 && wasabi) {
        wasabi->setMass(arg1, currentEA);
    }
}

void WASABIQtWindow::on_spinBoxUpdateRate_valueChanged(int arg1)
{
    if (arg1 > 0) {
        updateRate = arg1;
        if (wasabi) {
            wasabi->setUpdateRate(ui->spinBoxUpdateRate->value(), currentEA);
        }
        if (ui->checkBoxRun->isChecked()) {
            timer->start();
        }
    }
    else {
        updateRate = 0;
        timer->stop();
    }
}

void WASABIQtWindow::on_spinBoxAlpha_valueChanged(int arg1)
{
    if (arg1 >= 0 && wasabi) {
        wasabi->setAlpha(arg1, currentEA);
    }
}

void WASABIQtWindow::on_spinBoxBeta_valueChanged(int arg1)
{
    if (arg1 >= 0 && wasabi) {
        wasabi->setBeta(arg1, currentEA);
    }
}

void WASABIQtWindow::on_spinBoxFactor_valueChanged(int arg1)
{
    if (arg1 >= 0 && wasabi) {
        wasabi->setFactor(arg1, currentEA);
    }
}

void WASABIQtWindow::on_pushButtonMinus50_clicked()
{
    wasabi->emotionalImpulse(-50, currentEA);
}

void WASABIQtWindow::on_pushButtonPlus50_clicked()
{
    wasabi->emotionalImpulse(50, currentEA);
}

void WASABIQtWindow::on_pushButtonReset_clicked()
{
    wasabi->resetToZero(currentEA);
}

void WASABIQtWindow::on_verticalSliderP_valueChanged(int value)
{
    cogaEmotionalAttendee* ea = wasabi->getEAfromID(currentEA);
    if (!ea){
        return;
    }
    ui->checkBoxRun->setChecked(false);
    timer->stop();
    ea->resetForces();
    ea->setPValue(value);
    ea->EmoConPerson->updateAffectLikelihoods(true); // true forces recalculation of PAD-based likelihoods
    ea->updatePADstring();
    updateGUI(true);
}

void WASABIQtWindow::on_horizontalSliderA_valueChanged(int value)
{
    cogaEmotionalAttendee* ea = wasabi->getEAfromID(currentEA);
    if (!ea){
        return;
    }
    ui->checkBoxRun->setChecked(false);
    timer->stop();
    ea->resetForces();
    ea->setAValue(value);
    ea->EmoConPerson->updateAffectLikelihoods(true); // true forces recalculation of PAD-based likelihoods
    ea->updatePADstring();
    updateGUI(true);
}

void WASABIQtWindow::on_verticalSliderD_valueChanged(int value)
{
    cogaEmotionalAttendee* ea = wasabi->getEAfromID(currentEA);
    if (!ea){
        return;
    }
    ui->checkBoxRun->setChecked(false);
    timer->stop();
    ea->resetForces();
    ea->setDValue(value);
    ea->EmoConPerson->updateAffectLikelihoods(true); // true forces recalculation of PAD-based likelihoods
    ea->updatePADstring();
    updateGUI(true);
}

void WASABIQtWindow::on_treeViewAffectiveStates_activated(const QModelIndex &index)
{
    ui->pushButtonTrigger->setEnabled(false);
    ui->pushButtonShow->setEnabled(false);
    highlighted_as = NULL;
    QString evText = ea_model->itemFromIndex(index)->text();
    if (ea_model->parent(index).isValid()) {
        QString parentText = (ea_model->itemFromIndex(ea_model->parent(index)))->text();
        if (parentText == "Secondary" || parentText == "Primary") {
            std::cout << "WASABIGuiguiFrame::m_treeCtrlAffectiveStates_Activated: Emotion \"" << evText.toStdString() << "\" clicked." << std::endl;
            ui->pushButtonTrigger->setEnabled(true);
            ui->pushButtonShow->setEnabled(true);
            highlighted_as = wasabi->getEAfromID(currentEA)->EmoConPerson->getAffectiveStateByType(evText.toStdString());
        }
        else {
            std::cout << "WASABIQtWindow::on_treeViewAffectiveStates_activated: No emotion activated!" << std::endl;
        }
    }
}

void WASABIQtWindow::on_pushButtonTrigger_clicked()
{
    if (highlighted_as) {
        PrimaryEmotion* pe;
        pe = dynamic_cast<PrimaryEmotion*> (highlighted_as);
        SecondaryEmotion* se;
        se = dynamic_cast<SecondaryEmotion*> (highlighted_as);
        if (pe) {
            highlighted_as->trigger(10.0);
        }
        else if (se) {
            highlighted_as->trigger(); // -1 indicates SecondayEmotion's standard lifetime to be used
        }
    }
    else {
        std::cout << "myGuiFrame::m_buttonTriggerOnClick: No highlighted_as!" << std::endl;
    }
}


/** We expect messages (i.e. strings with a maximum length of 100 characters) that conform the following BNF:
 * <message>  ::= <senderID> '&' <command>
 * <senderID> ::= (any non-empty string)
 * <command>  ::= <add> | <trigger> | <impulse> | <dominance> | <remove> | <removeAll> | <getXmlFile>
 * <add>      ::= 'ADD' '&' <name> [ '&' <globalID> '&' <initfile> ]
 * <trigger>  ::= 'TRIGGER' '&' <targetID> '&' <affectiveStateName> [ '&' <lifetime> ]
 * <impulse>  ::= 'IMPULSE' '&' <targetID> '&' <impvalue>
 * <dominance>::= 'DOMINANCE' '&' <targetID> '&' <domvalue>
 * <name>     ::= (any non-empty string)
 * <initfile> ::= <padFile> | <xmlFile>
 * <padValue  ::= (any non-empty string, defaults to 'init', if not found)
 * <xmlValue> ::= (any non-empty string) '.xml'
 * <globalID> ::= (any non-empty string)
 * <targetID> ::= (any non-empty string)
 * <affectiveStateName> ::= (any non-empty string, must match a name of an emotion, though)
 * <lifetime> ::= (any double d with 0 <= d <= 100 or d == -1)
 * <impvalue> ::= (any integer i with -100 <= i <= 100 and i != 0)
 * <domvalue> ::= (any integer i with -100 <= i <= 100)
 * <remove>   ::= 'REMOVE' '&' '<targetID>
 * <removeAll>::= 'REMOVEALL'
 * <getXmlFile::= 'GETXMLFILE' '&' <xmlValue>
 * ------------------------------------------------------------------------------------------
 * ADD: Adds a new EmotionalAttendee (EA) to the simulation.
 *      For each EA an independent reference point (XYZ) is created
 *      The affective states are loaded from file <initfile>, if provided, or else from the default file 'init'.
 *      A globalID can optionally be provided as well, but an internal uid (int) is created as well.
 *      Returns: 'REPLY&ADD&OK&' <localID> only in case of success.
 * TRIGGER: Is used whenever an affective state's intensity is to be set to maximum for a certain amount of (life)time.
 *          For example, 'surprise' is initialized to have a baseIntensity of zero and, thus, needs to be triggered,
 *          before it can gain a positive awareness likelihood.
 *          Only triggering an emotion, however, might not be sufficient to 'activate' this emotion with a certain awareness likelihood.
 *          The PAD values of the reference point must be close enough to the emotion in question as well, while it has a non-zero intensity.
 *          This is what it means to have an 'emotion dynamics' instead of a purely rule-based and direct emotion elicitation.
 *          You need to provide the <targetID>, which is the uid returned by the ADD command or '1' for the default simulation of 'John Doe'.
 *          By specifying the <affectiveStateName> you tell the system, which "affective state" (i.e. primary or secondary emotion) to TRIGGER.
 *          Take care yourself that this affective state has been loaded from <initfile>.emo_pad before.
 *          Optionally, you might provide a lifetime (double) for this emotion. If no lifetime is given, the emotions standardLifetime will be used.
 * IMPULSE: Is used to drive the emotion dynamics in XYZ space itself.
 *          As soon as something positive or negative is detected to happen, you might use this command to tell the WASABI engine.
 *          Of course, the event can have happend to the agent/robot itself or to another person, i.e. another EmotionalAttendee (EA).
 *          In the former case, <targetID> should be set to '1', in the latter case, to that uid, which was returned after ADDing this EA to the simulation.
 *          The impulse must be within the range [-100,100] and should be an integer.
 * DOMINANCE: Is used to set the dominance value of the EA with ID <targetID> to any value
 *            between 100 (dominant) and -100 (submissive). As of June 2012 only the two extreme values make sense, though.
 *            Concerning the value of <targetID> the same applies as in case of IMPULSE explained above.
 * REMOVE : Removes a attendee with the given id;
 * REMOVEALL: Removes all attendees of the given owner.
 */
bool WASABIQtWindow::parseMessage(QString message) {
    std::cout << "WASABIQtWindow::parseMessage: message = '" << message.toStdString() << "'" << std::endl;
    QStringList str_list = message.split("&");
    if (str_list.size() < 3) {
        std::cerr << "WASABIQtWindow::parseMessage: too few tokens in message!" << std::endl;
        return false;
    }
    QString senderID = str_list.at(0); // will be ignored so far, but must be supplied
    QString command = str_list.at(1);
    QString targetID = str_list.at(2); // or <name> for ADD or <typeOfInfo> for REQUEST
    QString param1;
    QString param2;
    std::cout << "WASABIQtWindow::parseMessage: now checking '" << senderID.toStdString()
              << "' with command '" << command.toStdString()
              << "' with target '"
              << targetID.toStdString() << "'." << std::endl;
    double lifetime = -1;
    int eaID;
    int newLocalID = 0;
    int impulse; // P, A, D, X, Y, Z;
    cogaEmotionalAttendee* ea = wasabi->getEAfromID(currentEA);
    if (!ea) {
        std::cout << "WASABIQtWindow::parseMessage: no emotionalAttendee with ID '" << currentEA << "' found!" << std::endl;
        return false;
    }
    bool ok = false;
    switch (returnIndex(command.toStdString(), "ADD TRIGGER IMPULSE DOMINANCE REMOVE REMOVEALL GETXMLFILE")) {
    //<String senderID>|ADD|<String name==targetID>|<String globalID (optional)>
    //e.g. 'Robovie|ADD|Robovie|Robovie23' or 'Robovie1|ADD|Dylan F. Glas|120345_1' or simply 'Robovie|ADD|Chris'
    case 1://ADD <name> [ <globalID> <initfile> ]
        if (str_list.size() == 3) {
            newLocalID = addEmotionalAttendee(targetID, "undef"); // targetID is the 'real name' here, e.g. 'Dylan F. Glas'
        }
        else if (str_list.size() == 4){
            param1 = str_list.at(3); // <String globalID>
            newLocalID = addEmotionalAttendee(targetID, param1); // param1 is the globalID here, e.g. '120345_1'
        }
        else if (str_list.size() == 5){
            param1 = str_list.at(3); // <String globalID>
            param2 = str_list.at(4); // <String globalID>
            newLocalID = addEmotionalAttendee(targetID, param1); // param1 is the globalID here, e.g. '120345_1'
            ea = wasabi->getEAfromID(newLocalID);

            if(param2.endsWith(".xml")){
                if(ea != NULL){
                    ea->EmoConPerson->xmlFilename = param2.toStdString();
                    initEAbyXML(ea);
                    ea->setOwner(senderID.toStdString());
                } else {
                    qDebug()
                            << "MyApp::loadInitFile(): ERROR no ea with ID "
                            << newLocalID
                            << " found!"
                               ;
                }
            }
            else{
                QString param3 = param2.append(".emo_dyn");
                param2 = param2.append(".emo_pad");

                std::string dyn = param3.toStdString();
                std::string pad = param2.toStdString();

                if (ea != NULL) {
                    ea->EmoConPerson->dynFilename = dyn;
                    ea->EmoConPerson->padFilename = pad;
                    wasabi->initEA(ea);
                    ea->setOwner(senderID.toStdString());
                } else {
                    qDebug()
                            << "MyApp::loadInitFile(): ERROR no ea with ID "
                            << newLocalID
                            << " found!"
                               ;
                }
                updateGuiAfterAddingAgent(ea);
            }

        }
        if (newLocalID == 0) {
            return false;
        }

        break;
    case 2: //TRIGGER <String targetID> <String affectiveStateName> <double lifetime>
        if (str_list.size()==3) {
            std::cout << "WASABIQtWindow::parseMessage: not enough tokens for '"
                      << command.toStdString() << "'!" << std::endl;
            return false;
        }
        param1 = str_list.at(3); // <String affectiveStateName>
        if (str_list.size() == 4) {
            std::cout << "WASABIQtWindow::parseMessage: no lifetime for '"
                      << command.toStdString() << "', using standard lifetime." << std::endl;
            param2 = "-1";
        }
        else {
            param2 = str_list.at(4); // <double lifetime>
        }
        lifetime = param2.toDouble(&ok); //atof((const char*)param2.mb_str());
        if (!ok || (lifetime < 0 && lifetime != -1) || lifetime > 100) {
            std::cout
                    << "WASABIQtWindow::parseMessage: second parameter (double lifetime) of '"
                    << command.toStdString()
                    << "' is not in range ]0, 100] and not -1 (or no double at all)!"
                    << std::endl;
            return false;
        }

        //TODO CHECK THE ABOVE!!!
        eaID = targetID.toInt(&ok);//atoi((const char*)targetID.mb_str());
        std::cout << "WASABIQtWindow::parseMessage: eaID = " << eaID << std::endl;
        if (ok && eaID > 0) {
            ea = wasabi->getEAfromID(eaID);
        }
        if (!ea || !ea->EmoConPerson->triggerAS(param1.toStdString(), lifetime)) {
            std::cout << "WASABIQtWindow::parseMessage: Couldn't TRIGGER '" << param1.toStdString() << "'!" << std::endl;
            return false;
        }
        break;
    case 3: //IMPULSE <String targetID> <int impulse>
        if (str_list.size()==3) {
            std::cout << "WASABIQtWindow::parseMessage: not enough tokens for '"
                      << command.toStdString() << "'!" << std::endl;
            return false;
        }
        param1 = str_list.at(3);
        impulse = param1.toInt(&ok); //atoi((const char*)param1.mb_str());
        if (!ok || impulse < -100 || impulse > 100 || impulse == 0) {
            std::cout
                    << "WASABIQtWindow::parseMessage: second parameter (int impulse) of '"
                    << command.toStdString()
                    << "' is not in range [-100, 100] or 0 (or no integer at all)!"
                    << std::endl;
            return false;
        }



        //eaID = targetID.toInt(&ok);//atoi((const char*)targetID.mb_str());
        ea = wasabi->getEAfromID(targetID.toStdString());
        if(ea == 0)
            break;
        eaID = ea->getLocalID();



        std::cout << "WASABIQtWindow::parseMessage: eaID = " << eaID << std::endl;
        return wasabi->emotionalImpulse(impulse, eaID);
        break;
    case 4: //DOMINANCE <String targetID> <int impulse>
        if (str_list.size()==3) {
            std::cout << "WASABIQtWindow::parseMessage: not enough tokens for '"
                      << command.toStdString() << "'!" << std::endl;
            return false;
        }
        param1 = str_list.at(3);
        impulse = param1.toInt(&ok); //atoi((const char*)param1.mb_str());
        if (!ok || impulse < -100 || impulse > 100) {
            std::cout
                    << "WASABIQtWindow::parseMessage: second parameter (int impulse) of '"
                    << command.toStdString()
                    << "' is not in range [-100, 100] or 0 (or no integer at all)!"
                    << std::endl;
            return false;
        }



        //eaID = targetID.toInt(&ok);//atoi((const char*)targetID.mb_str());
        ea = wasabi->getEAfromID(targetID.toStdString());
        if(ea == 0)
            break;
        eaID = ea->getLocalID();


        std::cout << "WASABIQtWindow::parseMessage: eaID = " << eaID << std::endl;
        ea = wasabi->getEAfromID(eaID);
        if (!ea) {
            std::cerr << "WASABIQtWindow::parseMessage: unknown ea with ID '" << eaID
                      << "'!" << std::endl;
            return false;
        }
        ea->setDValue(impulse);
        return true;
        break;
    case 5: //REMOVE <String targetID>
        std::cout << "WASABIQtWindow::parseMessage: '" << "Removing attendee with global id "
                  << targetID.toStdString() << "'" << " and local id " << ea->getLocalID() << std::endl;

        //TODO this is just a workaround because otherwise WASABI crashes
        ea = wasabi->getEAfromID("undef1");
        currentEA = ea->getLocalID();

        if(!wasabi->removeAttendee(targetID.toStdString())){
            std::cerr << "WASABIQtWindow::parseMessage: Could not remove attendee with id '" << eaID
                      << "'!" << std::endl;
            return false;
        }

        return true;
        break;
    case 6: //REMOVEALL
        std::cout << "WASABIQtWindow::parseMessage: '" << "Removing all attendees of " << senderID.toStdString() << std::endl;

        //TODO this is just a workaround because otherwise WASABI crashes
        ea = wasabi->getEAfromID("undef1");
        currentEA = ea->getLocalID();

        wasabi->removeAllAttendeesOf(senderID.toStdString());
        //std::cout << "WASABIQtWindow::parseMessage: Removed " << attendeesRemoved << " attendees." << std::endl;

        return true;
        break;
    case 7: //GETXMLFILE
        std::cout << "WASABIQtWindow::parseMessage: '" << "Sending xml file." << std::endl;

        sendXmlFile(targetID);

        return true;
        break;
    default:
        std::cerr << "WASABIQtWindow::parseMessage: unknown command '" << command.toStdString()
                  << "'!" << std::endl;
        return false;
    }
    return true;
}

//Server stuff
void WASABIQtWindow::processPendingDatagrams()
{
    while (udpSocketReceiver->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocketReceiver->pendingDatagramSize());
        udpSocketReceiver->readDatagram(datagram.data(), datagram.size());
        std::cout << "Received datagram: [" << datagram.data() << "]" << std::endl;
        if (!ui->checkBoxReceiving->isChecked()) {
            std::cout << "WASABIQtWindow::processPendingDatagrams: 'Receiving' not checked, ignoring message" << std::endl;
            printNetworkMessage(datagram.data(), true, false, false);
            return;
        }
        printNetworkMessage(datagram.data(), true, parseMessage(datagram.data()), true);
    }
}

void WASABIQtWindow::broadcastDatagram() {

    if (ui->checkBox_senderMode_AL->isChecked()) {

        string timeStamp = getPackageTimeStamp();

        int i=0;
        while(i<=wasabi->emoAttendees.size()/50){

            int firstIndex = i*50;
            int lastIndex = firstIndex + 50;
            if(lastIndex > wasabi->emoAttendees.size()){
                lastIndex = wasabi->emoAttendees.size();
            }

            vector<cogaEmotionalAttendee*> subset(&(wasabi->emoAttendees[firstIndex]), &(wasabi->emoAttendees[lastIndex]));

            string padStrings = buildAffectedLikelihoodStrings(subset, timeStamp);

            sendAffectedLikelihood(padStrings);

            i++;
        }
    }
/*******
    if(ui->checkBox_senderMode_PAD){
        //QByteArray datagram = "SenderID&IMPULSE&1&" + QByteArray::number(messageNo);
        std::stringstream ostr;
        wasabi->getEAfromID(currentEA)->EmoConPerson->writeTransferable(ostr);
        QByteArray datagram = QByteArray(ostr.str().data(), ostr.str().size());
        if (udpSocketSender->writeDatagram(datagram.data(), datagram.size(), QHostAddress::Broadcast, sPort) != -1) {
            printNetworkMessage(datagram.data(), false, true);
        }
        else {
            printNetworkMessage(datagram.data(), false, false);
        }
    }
********/
    //EXTENSION1:

    if (ui->checkBox_senderMode_PADtrace->isChecked()) {
        sendEmoMlPadTrace();
    }
    //END OF EXTENSION1
}

string WASABIQtWindow::getPackageTimeStamp(){

    stringstream time;
    clock_t t = clock();
    if(t <= lastPackageTimeStamp){
        t = lastPackageTimeStamp+1;
    }
    lastPackageTimeStamp = t;
    time << t;
    return time.str();

}

std::string WASABIQtWindow::buildAffectedLikelihoodStrings(vector<cogaEmotionalAttendee*> attendees, string timeStamp){
    char buffer[33];
    itoa(attendees.size(),buffer,10);
    std::string padString;

    std::string padStrings = "timeStamp=";
    padStrings += timeStamp;
    padStrings += " ";
    padStrings += "total=";
    padStrings += buffer;
    padStrings += " ";
    std::vector<cogaEmotionalAttendee*>::iterator iter_ea;
    //const char* pEmoMlStr;
    for (iter_ea = attendees.begin(); iter_ea != attendees.end(); ++iter_ea){
        if (iter_ea != attendees.end() && iter_ea != attendees.begin()) {
            padStrings += " ";
        }
        cogaEmotionalAttendee* ea = (*iter_ea);
        if (wasabi->getPADString(padString, ea->getLocalID())) {
            myReplace(padString, "&", " ");
        } else {
            std::cerr << "WASABIQtWindow::printNetworkMessage: No padString found!" << std::endl;
        }
        ui->textEditOut->append(QString("(%0) %1: %2").arg(QTime::currentTime().toString())
                            .arg(ea->getName().c_str())
                            .arg(padString.c_str()));
        //padStrings += std::to_string(ea->getLocalID());
        itoa(ea->getLocalID(), buffer, 10);
        padStrings += "ID";
        padStrings += buffer;

        stringstream padValues;
        padValues << " P=" << ea->getPValue() << " A=" << ea->getAValue() << " D=" << ea->getDValue();

        padStrings += "=" + ea->getGlobalID() + " ( " + padString + padValues.str() + " )";
    }

    return padStrings;
}

void WASABIQtWindow::sendAffectedLikelihood(string padStrings){
    QByteArray datagram(padStrings.c_str());

    if (udpSocketSender->writeDatagram(datagram.data(), datagram.size(), QHostAddress::Broadcast, sPort) != -1) {
        printNetworkMessage(datagram.data(), false, true);
    }
    else {
        printNetworkMessage(datagram.data(), false, false);
    }
}

void WASABIQtWindow::sendEmoMlPadTrace(){
    std::vector<cogaEmotionalAttendee*>::iterator iter_ea;
    //const char* pEmoMlStr;
    for (iter_ea = wasabi->emoAttendees.begin(); iter_ea != wasabi->emoAttendees.end(); ++iter_ea){
        cogaEmotionalAttendee* ea = (*iter_ea);

        EmoMLString = composeEmoML(ea);
        //pEmoMlStr = EmoMLString.c_str();
        QByteArray emoMlDatagram(EmoMLString.c_str());
        //udpSocketSender->writeDatagram(emoMlDatagram.data(), emoMlDatagram.size(), QHostAddress::Broadcast, sPort);
        if (udpSocketSender->writeDatagram(emoMlDatagram.data(), emoMlDatagram.size(), QHostAddress::Broadcast, sPort) != -1) {
            printNetworkMessage(emoMlDatagram.data(), false, true);
            ea->resetBuffer();
        }
        else {
            printNetworkMessage(emoMlDatagram.data(), false, false);
        }
    }
}

void WASABIQtWindow::sendXmlFile(QString filename){

    QString xmlString;

    QFile inputFile("xml/" + filename);
    if (inputFile.open(QIODevice::ReadOnly))
    {
       QTextStream in(&inputFile);
       while ( !in.atEnd() )
       {
          xmlString.append(in.readLine());
       }
       inputFile.close();
    }

    QString startElement = "<emotionml ";

    QXmlStreamReader xmlReader(xmlString);
    xmlReader.readNextStartElement();
    QXmlStreamNamespaceDeclarations namespaces = xmlReader.namespaceDeclarations();
    for (int i=0; i<namespaces.size(); ++i){
        if(namespaces.at(i).prefix().toString() == "wasabi"){
            startElement += "xmlns:wasabi=\"" + namespaces.at(i).namespaceUri().toString()+ "\"";
        }
    }
    startElement = startElement.trimmed();
    startElement += ">";

    QDomDocument domTree;
    domTree.setContent(xmlString);
    QDomNodeList elementNodes = domTree.elementsByTagName("emotion");
    for (int i=0; i<elementNodes.count(); i++) {

        QDomNode node = elementNodes.item(i);
        QString nodeString;
        QTextStream stream(&nodeString);
        node.save(stream, 0);

        nodeString = "emotion: " + startElement + nodeString + "</emotionml>";

        QByteArray xmlDatagram(nodeString.toStdString().c_str());
        if (udpSocketSender->writeDatagram(xmlDatagram.data(), xmlDatagram.size(), QHostAddress::Broadcast, sPort) != -1) {
            printNetworkMessage(xmlDatagram.data(), false, true);
        }
        else {
            printNetworkMessage(xmlDatagram.data(), false, false);
        }

        //TODO Find a better solution. If there are sent too many packages at once, some get missed.
        usleep(1000);
    }
}

void WASABIQtWindow::printNetworkMessage(QString message, bool receive, bool success, bool parsed) {
    QString p;
    if (receive) {
        p = "Receive (";
        parsed ? p = p.append("parsed, ") : p.append("skipped, ");
    }
    else {
        p = "Transmit (";
    }
    success ? p = p.append("success)") : p.append("failure)");
    ui->textEditNetworkTraffic->append(QString("(%0) %1 [%2]").arg(QTime::currentTime().toString()).arg(p).arg(message));
    // HACK, please find a better place, perhaps we need to create an independent times (TODO)
}

void WASABIQtWindow::on_lineEditSenderPort_textEdited(const QString &arg1)
{
    bool ok = false;
    int tmp = arg1.toInt(&ok);
    if (ok && tmp > 500 && tmp < 65536) {
        sPort = tmp;
        udpSocketSender->bind(sPort);
    }
}

void WASABIQtWindow::on_spinBoxSendRate_valueChanged(int arg1)
{
    if (arg1 > 0 && arg1 <= 50) {
        updateRateSender = arg1;
        timerSender->setInterval(1000/updateRateSender);
    }
}

void WASABIQtWindow::on_checkBoxSending_stateChanged(int arg1)
{
    if (arg1 == 0) { // unchecked -> stop sending
        timerSender->stop();
    }
    else {
        timerSender->start();
    }
}

void WASABIQtWindow::actionAbout() {
    QMessageBox* about;
    about = new QMessageBox(QMessageBox::Information, "About WASABIQtGUI", QString("<p>Copyright (C) 2011 Christian Becker-Asano. <br>All rights reserved.<br>Contact: Christian Becker-Asano (christian@becker-asano.de)</p>This is version %0 of the Qt5-based Graphical User Interface (GUI) WASABIQtGUI, which depends on the shared libraries WASABIEngine and qwt (included as submodules) to run the WASABI Affect Simulation Architecture as described in the doctoral thesis of Christian Becker-Asano. It is licensed under the LGPL and its source can be obtained freely via GitHub.<p>For further information, please visit:<br> <a href='https://www.becker-asano.de/index.php/component/search/?searchword=WASABI'>https://www.becker-asano.de/index.php/component/search/?searchword=WASABI</a></p>").arg(CURRENT_VERSION), QMessageBox::Ok);
    about->setTextFormat(Qt::RichText);
    about->show();
}

void WASABIQtWindow::actionPAD_space() {
    if (ui->actionPAD_space->isChecked()){
        padWindow->show();
    }
    else {
        padWindow->hide();
    }
}

void WASABIQtWindow::actionPlot() {
    if (ui->actionPlot->isChecked()) {
        qwtPlotterWindow->show();
    }
    else {
        qwtPlotterWindow->hide();
    }
}

//EXTENSION2
std::string
WASABIQtWindow::composeEmoML(cogaEmotionalAttendee* ea)
{
    if (ea) {
        std::string trace1, trace2, trace3;
        //strUpdateRate = ea->intToString(updateRate);
        trace1 = ea->getPBuffer();
        trace2 = ea->getABuffer();
        trace3= ea-> getDBuffer();
        std::stringstream ssEmoML;
        ssEmoML << "<emotionml version=\"1.0\" xmlns=\"http://www.w3.org/2009/10/emotionml\"> <emotion dimension-set=\"http://www.w3.org/TR/emotion-voc/xml#pad-dimensions\"> <dimension name=\"pleasure\"> <trace freq=\"" << updateRate << "Hz\" samples= \"" << trace1 << "\"/> </dimension> <dimension name=\"arousal\"> <trace freq=\"" << updateRate << "Hz\" samples= \"" << trace2 << "\"/> </dimension> <dimension name=\"dominance\"> <trace freq=\"" << updateRate << "Hz\" samples=\"" << trace3 << "\"/> </dimension> </emotion> </emotionml>";
        return ssEmoML.str();
    }
    return "Error";
}
//END OF EXTENSION2

void WASABIQtWindow::on_pushButton_network_send_clicked()
{
    if (ui->lineEdit_network_send->text().isEmpty())
        return;
    QByteArray datagram;
    datagram.append(ui->lineEdit_network_send->text());
    int port = (ui->lineEditReceiverPort->text()).toInt();
    std::cout << "Sending to port " << port;
    //udpSocketSender->setPeerPort((ui->lineEditReceiverPort->text()).toInt());
    if (udpSocketSender->writeDatagram(datagram.data(), datagram.size(), QHostAddress::Broadcast, port) != -1) {
        printNetworkMessage(datagram.data(), false, true);
    }
    else {
        printNetworkMessage(datagram.data(), false, false);
    }
    std::cout << " done!" << std::endl;
}

void WASABIQtWindow::setPADspace(bool state)
{
    ui->actionPAD_space->setChecked(state);
}

void WASABIQtWindow::setQWT(bool state)
{
    ui->actionPlot->setChecked(state);
}

/*!
 * Initializing ea's using the xml file.
 */
bool WASABIQtWindow::initEAbyXML(cogaEmotionalAttendee* ea)
{
    QString filename = "xml/";
    filename = filename.append(ea->EmoConPerson->xmlFilename.c_str());
    QFile file;
    if (!(file.exists(filename))) {
        qDebug() << "WASABIQtWindow::initEAbyXML: file " << filename << " not found.";
        return false;
    }
    QXmlStreamReader xml;
    file.setFileName(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "WASABIQtWindow::initEAbyXML: Could not open " << filename << " read-only in text mode.";
        return false;
    }
    xml.setDevice(&file);
    if (xml.readNextStartElement()) {
        std::cout << "xml.name()='''" << xml.name().toString().toStdString() << "'''" << std::endl;
        if (xml.name() == "emotionml" && xml.attributes().value("version") == "1.0") {
            return readEmotionML(xml, ea);
        }
        else
            xml.raiseError(QObject::tr("The file is not an EMOTIONML version 1.0 file."));
    }
    return false;
}

bool WASABIQtWindow::readEmotionML(QXmlStreamReader& xml, cogaEmotionalAttendee* ea) {
    Q_ASSERT(xml.isStartElement() && xml.name() == "emotionml");
    bool returnValue = false;
    tmp_emodata.affect_polygons.clear();
    tmp_emodata.emoclass = "undef";
    while (xml.readNextStartElement()) {
        std::cout << "^^^xml.name()='''" << xml.name().toString().toStdString() << "'''" << std::endl;
        if (xml.name() == "info")
            returnValue = readInfo(xml, ea);
        else if (xml.name() == "emotion") {
            std::cout << "***WASABIQtWindow::readEmotionML: xml.name()='''" << xml.name().toString().toStdString() << "'''" << std::endl;
            returnValue = readEmotion(xml, ea);
            if (returnValue && tmp_emodata.emoclass != "undef") {
                //We got a primary or secondary emotion
                if (tmp_emodata.emoclass == "primary") {
                    std::vector<float> p_a_d_ID_max_min_baseInt;
                    p_a_d_ID_max_min_baseInt.push_back(tmp_emodata.pleasure);
                    p_a_d_ID_max_min_baseInt.push_back(tmp_emodata.arousal);
                    p_a_d_ID_max_min_baseInt.push_back(tmp_emodata.dominance);
                    p_a_d_ID_max_min_baseInt.push_back(-1); //MOOD_ID is obsolete without agent MAX!
                    p_a_d_ID_max_min_baseInt.push_back(tmp_emodata.sat_threshold);
                    p_a_d_ID_max_min_baseInt.push_back(tmp_emodata.act_threshold);
                    p_a_d_ID_max_min_baseInt.push_back(tmp_emodata.base_intensity);
                    ea->EmoConPerson->buildPrimaryEmotion(p_a_d_ID_max_min_baseInt, tmp_emodata.category, tmp_emodata.decay);
                }
                else if (tmp_emodata.emoclass == "secondary" && tmp_emodata.affect_polygons.size() > 0) {
                    SecondaryEmotion* se = new SecondaryEmotion();
                    EmotionDynamics* me = dynamic_cast<EmotionDynamics*>(ea->EmoConPerson);
                    if (me) {
                        se->setEmotionContainer(me);
                    }
                    se->addPolygon(tmp_emodata.affect_polygons);
                    if (!se->setStandardLifetime((double)tmp_emodata.lifetime) || !se->setLifetime((double)tmp_emodata.lifetime)) {
                        cerr << "WASABIQtWindow::readEmotionML: WARNING invalid lifetime " << (double)tmp_emodata.lifetime << endl;
                    }
                    if (!se->setBaseIntensity((double)tmp_emodata.base_intensity)) {
                        cerr << "SecondaryEmotion::loadFromFile: WARNING invalid baseIntensity " << (double)tmp_emodata.base_intensity << endl;
                    }
                    se->type = tmp_emodata.category;
                    if (tmp_emodata.decay == "linear") {
                        se->setDecayFunction(SecondaryEmotion::LINEAR);
                    }
                    else if (tmp_emodata.decay == "none") {
                        se->setDecayFunction(SecondaryEmotion::NONE);
                    }
                    else if (tmp_emodata.decay == "exponential") {
                        se->setDecayFunction(SecondaryEmotion::EXPONENTIAL);
                    }
                    else if (tmp_emodata.decay == "cosine") {
                        se->setDecayFunction(SecondaryEmotion::COSINE);
                    }
                    ea->EmoConPerson->affectiveStates.push_back(se);
                }
            }
        }
        else
            xml.skipCurrentElement();
    }
    return returnValue;
}

bool WASABIQtWindow::readInfo(QXmlStreamReader& xml, cogaEmotionalAttendee* ea) {
    Q_ASSERT(xml.isStartElement() && xml.name() == "info");
    xml.readNext();
    std::cout << xml.tokenType() << " tokenType!" << std::endl;
    while (!(xml.tokenType() == QXmlStreamReader::EndElement &&
             xml.name() == "info")) {
        if (xml.name() == "" || xml.tokenType() == QXmlStreamReader::EndElement) {
            xml.readNext();
            continue;
        }
        std::cout << "WASABIQtWindow::readInfo: xml.name()='''" << xml.name().toString().toStdString() << "'''" << std::endl;
        //std::cout << "WASABIQtWindow::readInfo: xml.namespaceUri()='''" << xml.namespaceUri().toString().toStdString() << "'''" << std::endl;
        //This results in the http address!
        if (xml.name() == "parameter") {
            if ( xml.namespaceUri().toString().toStdString() != "http://www.becker-asano.de/WASABI/Shema/WASABI") {
                 std::cout << "WASABIQtWindow::readInfo: wrong namespaceUri, only http://www.becker-asano.de/WASABI/Shema/WASABI allowed:" << std::endl;
                 xml.readNext();
                 continue;
            }
            if (xml.attributes().hasAttribute("type") && xml.attributes().value("type") == "dynamic") {
                // These are the general dynamic infos
                std::cout << "xml.attributes().value(\"name\") = " << xml.attributes().value("name").toString().toStdString() << std::endl;
                QStringRef nameValue = xml.attributes().value("name");
                bool ok;
                int value;
                switch (returnIndex(nameValue.toString().toStdString(), "xTens yTens slope mass xReg yReg boredom prevalence")) {
                case 1: //xTens
                    value = xml.attributes().value("value").toInt(&ok);
                    if (ok) {
                        std::cout << "setting xTens for EA " << ea->getLocalID() << std::endl;
                        ea->EmoConPerson->xTens = value;
                    }
                    else {
                        qDebug() << "WASABIQtWindow::readInfo: error converting " << xml.attributes().value("value") << " to integer.";
                    }
                    break;
                 case 2: //yTens
                    value = xml.attributes().value("value").toInt(&ok);
                    if (ok) {
                        std::cout << "setting yTens for EA " << ea->getLocalID() << std::endl;
                        ea->EmoConPerson->yTens = value;
                    }
                    else {
                        qDebug() << "WASABIQtWindow::readInfo: error converting " << xml.attributes().value("value") << " to integer.";
                    }
                    break;
                case 3: //slope
                   value = xml.attributes().value("value").toInt(&ok);
                   if (ok) {
                       std::cout << "setting slope for EA " << ea->getLocalID() << std::endl;
                       ea->EmoConPerson->slope = value;
                   }
                   else {
                       qDebug() << "WASABIQtWindow::readInfo: error converting " << xml.attributes().value("value") << " to integer.";
                   }
                   break;
                case 4: //mass
                   value = xml.attributes().value("value").toInt(&ok);
                   if (ok) {
                       std::cout << "setting mass for EA " << ea->getLocalID() << std::endl;
                       ea->EmoConPerson->mass = value;
                   }
                   else {
                       qDebug() << "WASABIQtWindow::readInfo: error converting " << xml.attributes().value("value") << " to integer.";
                   }
                   break;
                case 5: //xReg
                   value = xml.attributes().value("value").toInt(&ok);
                   if (ok) {
                       std::cout << "setting xReg for EA " << ea->getLocalID() << std::endl;
                       ea->EmoConPerson->xReg = value;
                   }
                   else {
                       qDebug() << "WASABIQtWindow::readInfo: error converting " << xml.attributes().value("value") << " to integer.";
                   }
                   break;
                case 6: //yReg
                   value = xml.attributes().value("value").toInt(&ok);
                   if (ok) {
                       std::cout << "setting yReg for EA " << ea->getLocalID() << std::endl;
                       ea->EmoConPerson->yReg = value;
                   }
                   else {
                       qDebug() << "WASABIQtWindow::readInfo: error converting " << xml.attributes().value("value") << " to integer.";
                   }
                   break;
                case 7: //boredom
                   value = xml.attributes().value("value").toInt(&ok);
                   if (ok) {
                       std::cout << "setting boredom for EA " << ea->getLocalID() << std::endl;
                       ea->EmoConPerson->boredom = value;
                   }
                   else {
                       qDebug() << "WASABIQtWindow::readInfo: error converting " << xml.attributes().value("value") << " to integer.";
                   }
                   break;
                case 8: //prevalence
                   value = xml.attributes().value("value").toInt(&ok);
                   if (ok) {
                       std::cout << "setting prevalence for EA " << ea->getLocalID() << std::endl;
                       ea->EmoConPerson->prevalence = value;
                   }
                   else {
                       qDebug() << "WASABIQtWindow::readInfo: error converting " << xml.attributes().value("value") << " to integer.";
                   }
                   break;
                default:
                    qDebug() << "WASABIQtWindow::readInfo: error reading unknown value " << xml.attributes().value("value") << "!";
                }
                //xml.skipCurrentElement();
            }
            else //i.e. if !(xml.attributes().hasAttribute("type") && xml.attributes().value("type") == "dynamic")
            {
                std::cout << "WASABIQtWindow::readInfo: error: only 'parameter' tags with attribute with 'type=dynamic' allowed on top level." << std::endl;
            }
        }
        else if (xml.name() == "primary") {
            tmp_emodata.emoclass = "primary";
            // We have to parse a primary emotion
            xml.readNext();
            while (!(xml.tokenType() == QXmlStreamReader::EndElement &&
                     xml.name() == "primary")) {
                if (xml.name() == "" || xml.tokenType() == QXmlStreamReader::EndElement) {
                    xml.readNext();
                    continue;
                }
                std::cout << "xml.name()='''" << xml.name().toString().toStdString() << "''', tokenType = " << xml.tokenType() << std::endl;
                if (xml.name() == "parameter") {
                    std::cout << "xml.attributes().value(\"name\") = " << xml.attributes().value("name").toString().toStdString() << std::endl;
                    QStringRef nameValue = xml.attributes().value("name");
                    //std::cout << "xml.namespaceUri()='''" << xml.namespaceUri().toString().toStdString() << "'''" << std::endl;
                    if ( xml.namespaceUri().toString().toStdString() != "http://www.becker-asano.de/WASABI/Shema/WASABI") {
                         std::cout << "WASABIQtWindow::readInfo: wrong namespaceUri, only http://www.becker-asano.de/WASABI/Shema/WASABI allowed:" << std::endl;
                         xml.readNext();
                         continue;
                    }
                    bool ok;
                    float value;
                    switch (returnIndex(nameValue.toString().toStdString(), "base_intensity act_threshold sat_threshold decay")) {
                    case 1: //base_intensity
                        value = xml.attributes().value("value").toFloat(&ok);
                        if (ok) {
                            std::cout << "setting base_intensity for EA " << ea->getLocalID() << std::endl;
                            tmp_emodata.base_intensity = value;
                        }
                        else {
                            qDebug() << "WASABIQtWindow::readInfo: error converting " << xml.attributes().value("value") << " to float.";
                        }
                        break;
                     case 2: //act_threshold
                        value = xml.attributes().value("value").toFloat(&ok);
                        if (ok) {
                            std::cout << "setting activation threshold for EA " << ea->getLocalID() << std::endl;
                            tmp_emodata.act_threshold = value;
                        }
                        else {
                            qDebug() << "WASABIQtWindow::readInfo: error converting " << xml.attributes().value("value") << " to float.";
                        }
                        break;
                    case 3: //sat_threshold
                       value = xml.attributes().value("value").toFloat(&ok);
                       if (ok) {
                           std::cout << "setting saturation threshold for EA " << ea->getLocalID() << std::endl;
                           tmp_emodata.sat_threshold = value;
                       }
                       else {
                           qDebug() << "WASABIQtWindow::readInfo: error converting " << xml.attributes().value("value") << " to float.";
                       }
                       break;
                    case 4: //decay
                       std::cout << "setting decay function for EA " << ea->getLocalID() << std::endl;
                       tmp_emodata.decay = xml.attributes().value("value").toString().toStdString();
                       break;
                    default:
                        qDebug() << "WASABIQtWindow::readInfo: (primary) error reading incorrect parameter value " << xml.attributes().value("value") << "!";
                    }
                }
                xml.readNext();
            }
        }
        else if (xml.name() == "secondary") {
            // We have to parse a primary emotion
            tmp_emodata.emoclass = "secondary";
            tmp_emodata.affect_polygons.clear();
            xml.readNext();
            while (!(xml.tokenType() == QXmlStreamReader::EndElement &&
                     xml.name() == "secondary") && xml.tokenType() != QXmlStreamReader::Invalid) {
                if (xml.name() == "" || xml.tokenType() == QXmlStreamReader::EndElement) {
                    xml.readNext();
                    continue;
                }
                std::cout << "xml.name()='''" << xml.name().toString().toStdString() << "''', tokenType = " << xml.tokenType() << std::endl;
                //std::cout << "xml.namespaceUri()='''" << xml.namespaceUri().toString().toStdString() << "'''" << std::endl;
                if ( xml.namespaceUri().toString().toStdString() != "http://www.becker-asano.de/WASABI/Shema/WASABI") {
                     std::cout << "WASABIQtWindow::readInfo: wrong namespaceUri, only http://www.becker-asano.de/WASABI/Shema/WASABI allowed:" << std::endl;
                     xml.readNext();
                     continue;
                }
                if (xml.name() == "parameter") {
                    std::cout << "xml.attributes().value(\"name\") = " << xml.attributes().value("name").toString().toStdString() << std::endl;
                    QStringRef nameValue = xml.attributes().value("name");
                    bool ok;
                    float value;
                    switch (returnIndex(nameValue.toString().toStdString(), "base_intensity lifetime decay")) {
                    case 1: //base_intensity
                        value = xml.attributes().value("value").toFloat(&ok);
                        if (ok) {
                            std::cout << "setting base_intensity for EA " << ea->getLocalID() << std::endl;
                            tmp_emodata.base_intensity = value;
                        }
                        else {
                            qDebug() << "WASABIQtWindow::readInfo: error converting " << xml.attributes().value("value") << " to float.";
                        }
                        break;
                     case 2: //lifetime
                        value = xml.attributes().value("value").toFloat(&ok);
                        if (ok) {
                            std::cout << "setting lifetime threshold for EA " << ea->getLocalID() << std::endl;
                            tmp_emodata.lifetime = value;
                        }
                        else {
                            qDebug() << "WASABIQtWindow::readInfo: error converting " << xml.attributes().value("value") << " to float.";
                        }
                        break;
                    case 3: //decay
                       std::cout << "setting decay function for EA " << ea->getLocalID() << std::endl;
                       tmp_emodata.decay = xml.attributes().value("value").toString().toStdString();
                       break;
                    default:
                        qDebug() << "WASABIQtWindow::readInfo: (secondary) error reading incorrect parameter value " << xml.attributes().value("value") << "!";
                    }
                }
                else if(xml.name() == "polygon"){
                    // We have to parse a primary emotion
                    QStringRef type = xml.attributes().value("type");
                    if (type != "QUAD") {
                        std::cout << "WASABIQtWindow::readInfo: wrong type '''" << type.toString().toStdString() << "''' for polygon" << std::endl;
                        break;
                    }
                    std::vector<AffectVertex*> affect_vertices;
                    xml.readNext();
                    while (!(xml.tokenType() == QXmlStreamReader::EndElement &&
                             xml.name() == "polygon")) {
                        if (xml.name() == "" || xml.tokenType() == QXmlStreamReader::EndElement) {
                            xml.readNext();
                            continue;
                        }
                        std::cout << "xml.name()='''" << xml.name().toString().toStdString() << "''', tokenType = " << xml.tokenType() << std::endl;
                        //std::cout << "xml.namespaceUri()='''" << xml.namespaceUri().toString().toStdString() << "'''" << std::endl;
                        if ( xml.namespaceUri().toString().toStdString() != "http://www.becker-asano.de/WASABI/Shema/WASABI") {
                             qDebug() << "WASABIQtWindow::readInfo: wrong namespaceUri, only http://www.becker-asano.de/WASABI/Shema/WASABI allowed:";
                             xml.readNext();
                             continue;
                        }
                        if (xml.name() == "vertex") {

                            if (xml.attributes().hasAttribute("pleasure") && xml.attributes().hasAttribute("arousal") && xml.attributes().hasAttribute("dominance") && xml.attributes().hasAttribute("intensity")) {
                                float pad_f[3]; //pleasure, arousal, dominance;
                                int pad[3];
                                float intensity;
                                bool ok;
                                pad_f[0] = xml.attributes().value("pleasure").toFloat(&ok);
                                if (ok) {
                                    pad[0] = (int)((pad_f[0] * 2 - 1.0) * 100); // old range [0.0, 1.0], new range [-100, 100]
                                    pad_f[1] = xml.attributes().value("arousal").toFloat(&ok);
                                    if (ok) {
                                        pad[1] = (int)((pad_f[1] * 2 - 1.0) * 100);
                                        pad_f[2] = xml.attributes().value("dominance").toFloat(&ok);
                                        if (ok) {
                                            pad[2] = (int)((pad_f[2] * 2 - 1.0) * 100);
                                            intensity = xml.attributes().value("intensity").toFloat(&ok);
                                            if (ok) {
                                                affect_vertices.push_back(new AffectVertex(pad, (double)intensity));
                                            }
                                        }
                                    }
                                }
                                if (!ok) {
                                    std::cout << "WASABIQtWindow::readInfo: at least one dimension or intensity was not a float value!" << std::endl;
                                }
                            }
                            else {
                                std::cout << "WASABIQtWindow::readInfo: at least one dimension or intensity is missing in vertex defnition" << std::endl;
                            }
                        }
                        xml.readNext();
                    }
                    if (affect_vertices.size() > 0) {
                        AffectPolygon* ap = new AffectPolygon(affect_vertices, "QUAD");
                        tmp_emodata.affect_polygons.push_back(ap);
                    }
                }
                xml.readNext();
            }
        }
        xml.readNext();
    }
    return true;
}

bool WASABIQtWindow::readEmotion(QXmlStreamReader& xml, cogaEmotionalAttendee* ea) {
    Q_ASSERT(xml.isStartElement() && xml.name() == "emotion");
    bool returnValue = false;;
    xml.readNext();
    while (!(xml.tokenType() == QXmlStreamReader::EndElement &&
             xml.name() == "emotion")) {
        if (xml.name() == "" || xml.tokenType() == QXmlStreamReader::EndElement) {
            xml.readNext();
            continue;
        }
        std::cout << "WASABIQtWindow::readEmotion xml.name()='''" << xml.name().toString().toStdString() << "'''" << std::endl;
        if (xml.name() == "info") {
            std::cout << "------------------inner readInfo start--------------------" << std::endl;
            returnValue = readInfo(xml, ea);
            std::cout << "------------------inner readInfo end  --------------------" << std::endl;
        }
        else if (xml.name() == "category") {
            std::string attrname = xml.attributes().value("name").toString().toStdString();
            std::cout << "WASABIQtWindow::readEmotion xml.attributes().value(\"name\") = " << attrname << std::endl;
            if (attrname.length() == 0) {
                std::cerr << "Attribute " << attrname << ".length() == 0!, bailing out." << std::endl;
                returnValue = false;
                break;
            }
            tmp_emodata.category = xml.attributes().value("name").toString().toStdString();
            returnValue = true;
        }
        else if (xml.name() == "dimension") {
            std::string attrname = xml.attributes().value("name").toString().toStdString();
            std::cout << "WASABIQtWindow::readEmotion xml.attributes().value(\"name\") = " << attrname << std::endl;
            if (attrname.length() == 0) {
                std::cerr << "Attribute " << attrname << ".length() == 0!, bailing out." << std::endl;
                returnValue = false;
                break;
            }
            bool ok;
            float value;
            // We have to rescale these values from [0, 1] to [-1, 1] !!!
            switch (returnIndex(attrname, "pleasure arousal dominance")) {
            case 1: //pleasure
                value = xml.attributes().value("value").toFloat(&ok);
                if (ok) {
                    value = value * 2 -1;
                    std::cout << "setting pleasure for EA " << ea->getLocalID() << " with emotion " << tmp_emodata.category << "." << std::endl;
                    tmp_emodata.pleasure = value;
                }
                else {
                    qDebug() << "WASABIQtWindow::readInfo: error converting " << xml.attributes().value("value") << " to float.";
                }
                break;
             case 2: //arousal
                value = xml.attributes().value("value").toFloat(&ok);
                if (ok) {
                    value = value * 2 -1;
                    std::cout << "setting arousal for EA " << ea->getLocalID() << " with emotion " << tmp_emodata.category << "." << std::endl;
                    tmp_emodata.arousal = value;
                }
                else {
                    qDebug() << "WASABIQtWindow::readInfo: error converting " << xml.attributes().value("value") << " to float.";
                }
                break;
            case 3: //dominance
               value = xml.attributes().value("value").toFloat(&ok);
               if (ok) {
                   value = value * 2 -1;
                   std::cout << "setting dominance for EA " << ea->getLocalID() << " with emotion " << tmp_emodata.category << "." << std::endl;
                   tmp_emodata.dominance = value;
               }
               else {
                   qDebug() << "WASABIQtWindow::readInfo: error converting " << xml.attributes().value("value") << " to float.";
               }
               break;
            default:
                qDebug() << "WASABIQtWindow::readInfo: error reading incorrect parameter value " << xml.attributes().value("value") << "!";
            }
            returnValue = true;
        }
        //else {
            //xml.skipCurrentElement();
        //}
        //xml.skipCurrentElement();
        xml.readNext();
        if (xml.tokenType() == QXmlStreamReader::StartElement) {
            std::cout << "WASABIQtWindow::readEmotion StartElement with name " << xml.name().toString().toStdString() << std::endl;
        }
        if (xml.tokenType() == QXmlStreamReader::Invalid) {
            std::cerr << "WASABIQtWindow::readEmotion Invalid element, bailing out! " << std::endl;
            returnValue = false;
            break;
        }
    }
    return returnValue;
}
