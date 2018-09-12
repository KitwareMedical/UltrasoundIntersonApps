/********************************************************************************
** Form generated from reading UI file 'PTX.ui'
**
** Created by: Qt User Interface Compiler version 5.10.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PTXUILAYOUT_H
#define PTXUILAYOUT_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QSplitter>

#include <iostream>

QT_BEGIN_NAMESPACE

class PTXUILayout  
{
public:
    QWidget *topWidget;
    QWidget *bottomWidget;
    QSplitter *splitter;
    
    QHBoxLayout *topLayout;
    QVBoxLayout *controlPanelVertical;
    QGridLayout *controlPanel;
    QComboBox *dropDown_Frequency;
    QLabel *label_Frequency;
    QSpinBox *spinBox_Depth;
    QLabel *label_Power;
    QSpinBox *spinBox_Power;
    QLabel *label_Depth;
    QPushButton *pushButton_Save;
    QPushButton *pushButton_ConnectProbe;
    QLabel *label_BModeImage;
    QFrame *line1;
    QFrame *line2;
    QFrame *line3;
    QCheckBox *checkbox_Overlay;
    
    QHBoxLayout *bottomLayout;
    QLabel *label_MMode1;
    QLabel *label_MMode2;
    QLabel *label_MMode3;
    
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {

        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(1140, 913);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(MainWindow->sizePolicy().hasHeightForWidth());
        MainWindow->setSizePolicy(sizePolicy);

        QFont font;
        font.setPointSize(10);
        MainWindow->setFont(font);
         
        splitter = new QSplitter( MainWindow );
        splitter->setOrientation( Qt::Vertical );
        splitter->setHandleWidth(7);
        const QString splitterSheetV =  \
"QSplitter::handle:vertical {	\
	border-top: 2px groove gray;	\
	border-bottom: 2px groove gray;	\
	margin: 2px 2px;			\
}";
        splitter->setStyleSheet( splitterSheetV );   

        topWidget = new QWidget();
        topWidget->setObjectName( QStringLiteral("topWidget") );
        splitter->addWidget( topWidget );
 
        topLayout = new QHBoxLayout();
        topLayout->setObjectName(QStringLiteral("topLayout"));
        topWidget->setLayout( topLayout );       
 
        controlPanel = new QGridLayout();
        controlPanel->setObjectName( QStringLiteral("controlPanel") );
        controlPanel->setSizeConstraint( QLayout::SetFixedSize );
        controlPanel->setColumnStretch( 0, 0 );
        controlPanel->setRowStretch( 0, 0 );

        controlPanelVertical = new QVBoxLayout();
        topLayout->addLayout( controlPanelVertical );

        QFont font1;
        font1.setPointSize(10);
        font1.setBold(false);
        font1.setWeight(50);

        dropDown_Frequency = new QComboBox();
        dropDown_Frequency->addItem(QString("3.5 Mhz"));
        dropDown_Frequency->addItem(QString("5.0 Mhz"));
        dropDown_Frequency->addItem(QString("7.5 Mhz"));
        dropDown_Frequency->setObjectName(QStringLiteral("dropDown_Frequency"));
        dropDown_Frequency->setMaximumSize(QSize(150, 30));
        dropDown_Frequency->setInputMethodHints(Qt::ImhNone);
        dropDown_Frequency->setFont( font1 );
        controlPanel->addWidget(dropDown_Frequency, 3, 1, 1, 1, Qt::AlignTop);

        label_Frequency = new QLabel();
        label_Frequency->setObjectName(QStringLiteral("label_Frequency"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(label_Frequency->sizePolicy().hasHeightForWidth());
        label_Frequency->setSizePolicy(sizePolicy1);
        label_Frequency->setMaximumSize(QSize(200, 30));

        label_Frequency->setFont(font1);
        label_Frequency->setAlignment(Qt::AlignBottom|Qt::AlignLeading|Qt::AlignLeft);

        controlPanel->addWidget(label_Frequency, 3, 0, 1, 1, Qt::AlignTop);

        spinBox_Depth = new QSpinBox();
        spinBox_Depth->setObjectName(QStringLiteral("spinBox_Depth"));
        spinBox_Depth->setMaximumSize(QSize(150, 30));
        spinBox_Depth->setMinimum(10);
        spinBox_Depth->setMaximum(200);
        spinBox_Depth->setSingleStep(5);
        spinBox_Depth->setValue(100);
        spinBox_Depth->setFont( font1 );

        controlPanel->addWidget(spinBox_Depth, 2, 1, 1, 1, Qt::AlignTop);

        label_Power = new QLabel();
        label_Power->setObjectName(QStringLiteral("label_Power"));
        label_Power->setMaximumSize(QSize(200, 30));
        label_Power->setFont( font1 );

        controlPanel->addWidget(label_Power, 1, 0, 1, 1, Qt::AlignTop);

        spinBox_Power = new QSpinBox();
        spinBox_Power->setObjectName(QStringLiteral("spinBox_Power"));
        spinBox_Power->setMaximumSize(QSize(150, 30));
        spinBox_Power->setMaximum(10);
        spinBox_Power->setMinimum(0);
        spinBox_Power->setValue(5);
        spinBox_Power->setFont( font1 );
        controlPanel->addWidget(spinBox_Power, 1, 1, 1, 1, Qt::AlignTop);

        label_Depth = new QLabel();
        label_Depth->setObjectName(QStringLiteral("label_Depth"));
        label_Depth->setMaximumSize(QSize(200, 30));
        label_Depth->setFont(font1);
        label_Depth->setAlignment(Qt::AlignBottom|Qt::AlignLeading|Qt::AlignLeft);
        label_Depth->setFont( font1 );
        controlPanel->addWidget(label_Depth, 2, 0, 1, 1, Qt::AlignTop);

        pushButton_Save = new QPushButton();
        pushButton_Save->setObjectName(QStringLiteral("pushButton_Save"));
        //pushButton_Save->setMaximumSize(QSize(200, 40));
        QFont font2;
        font2.setPointSize(12);
        pushButton_Save->setFont(font2);
        //controlPanel->addWidget(pushButton_Save, 0, 1, 1, 1, Qt::AlignTop);

        pushButton_ConnectProbe = new QPushButton();
        pushButton_ConnectProbe->setCheckable( true );
        pushButton_ConnectProbe->setObjectName(QStringLiteral("pushButton_ConnectProbe"));
        //pushButton_ConnectProbe->setMaximumSize(QSize(200, 40));
        pushButton_ConnectProbe->setFont( font2 );
        //controlPanel->addWidget(pushButton_ConnectProbe, 0, 0, 1, 1, Qt::AlignTop);

        controlPanelVertical->addWidget( pushButton_ConnectProbe );
        
        line1 = new QFrame();
        line1->setFrameShape( QFrame::HLine );
        line1->setFrameShadow( QFrame::Sunken );
        controlPanelVertical->addWidget(line1);

        controlPanelVertical->addLayout( controlPanel );

        line2 = new QFrame();
        line2->setFrameShape( QFrame::HLine );
        line2->setFrameShadow( QFrame::Sunken );
        controlPanelVertical->addWidget(line2);
        
        checkbox_Overlay = new QCheckBox("Detect Pneumothorax MMode");
        checkbox_Overlay->setFont( font2 );
        controlPanelVertical->addWidget( checkbox_Overlay );
        
        controlPanelVertical->addStretch( 0 );
        
        line3 = new QFrame();
        line3->setFrameShape( QFrame::HLine );
        line3->setFrameShadow( QFrame::Sunken );
        controlPanelVertical->addWidget(line2);
        
        controlPanelVertical->addWidget( pushButton_Save );

        topLayout->addStretch( 0 );

        label_BModeImage = new QLabel();
        label_BModeImage->setObjectName(QStringLiteral("label_BModeImage"));
        sizePolicy.setHeightForWidth(label_BModeImage->sizePolicy().hasHeightForWidth());
        label_BModeImage->setSizePolicy(sizePolicy);
        label_BModeImage->setMaximumSize(QSize(16777215, 401000));
        label_BModeImage->setFrameShape(QFrame::Box);
        label_BModeImage->setFrameShadow(QFrame::Sunken);
        label_BModeImage->setText(QStringLiteral(""));
        label_BModeImage->setTextFormat(Qt::AutoText);
        label_BModeImage->setAlignment(Qt::AlignCenter);

        topLayout->addWidget(label_BModeImage);

        topLayout->addStretch( 0 );

         
        bottomWidget = new QWidget();
        bottomWidget->setObjectName( QStringLiteral("bottomWidget") );
        
        bottomLayout = new QHBoxLayout();
        bottomLayout->setObjectName(QStringLiteral("bottomLayout"));
        bottomWidget->setLayout( bottomLayout ) ;  

        

        label_MMode1 = new QLabel();
        label_MMode1->setObjectName(QStringLiteral("label_MMode1"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(label_MMode1->sizePolicy().hasHeightForWidth());
        label_MMode1->setSizePolicy(sizePolicy2);
        //label_MMode1->setMinimumSize(QSize(0, 300));
        label_MMode1->setFrameShape(QFrame::Box);
        label_MMode1->setFrameShadow(QFrame::Sunken);
        bottomLayout->addWidget(label_MMode1);

        label_MMode2 = new QLabel();
        label_MMode2->setObjectName(QStringLiteral("label_MMode2"));
        label_MMode2->setFrameShape(QFrame::Box);
        label_MMode2->setFrameShadow(QFrame::Sunken);
        bottomLayout->addWidget(label_MMode2);

        label_MMode3 = new QLabel();
        label_MMode3->setObjectName(QStringLiteral("label_MMode3"));
        label_MMode3->setFrameShape(QFrame::Box);
        label_MMode3->setFrameShadow(QFrame::Sunken);
        bottomLayout->addWidget(label_MMode3);


        splitter->addWidget( bottomWidget );


        MainWindow->setCentralWidget( splitter );
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QStringLiteral("menubar"));
        menubar->setGeometry(QRect(0, 0, 1140, 20));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QStringLiteral("statusbar"));
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        //QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", nullptr));
        dropDown_Frequency->setItemText(0, QApplication::translate("MainWindow", "5 Mhz", nullptr));
        dropDown_Frequency->setItemText(1, QApplication::translate("MainWindow", "7.5 Mhz", nullptr));
        dropDown_Frequency->setItemText(2, QApplication::translate("MainWindow", "10 Mhz", nullptr));
        dropDown_Frequency->setItemText(3, QApplication::translate("MainWindow", "15 Mhz", nullptr));

        dropDown_Frequency->setCurrentText(QApplication::translate("MainWindow", "5 Mhz", nullptr));
        label_Frequency->setText(QApplication::translate("MainWindow", "Frequency (MHz) :", nullptr));
        label_Power->setText(QApplication::translate("MainWindow", "Power", nullptr));
        label_Depth->setText(QApplication::translate("MainWindow", "Depth:", nullptr));
        pushButton_Save->setText(QApplication::translate("MainWindow", "Save", nullptr));
        pushButton_ConnectProbe->setText(QApplication::translate("MainWindow", "Start Scan", nullptr));
    } // retranslateUi

};

QT_END_NAMESPACE

#endif // UI_PTX_H
