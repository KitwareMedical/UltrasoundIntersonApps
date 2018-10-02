/********************************************************************************
** Form generated from reading UI file 'PTX.ui'
**
** Created by: Qt User Interface Compiler version 5.10.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PTXUISAVEDIALOG_H
#define PTXUISAVEDIALOG_H

#include "PTXPatientData.h"

#include <QtWidgets/QDialog>
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
#include <QtWidgets/QLineEdit>
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

#include <iostream>

QT_BEGIN_NAMESPACE

class PTXUISaveDialog : public QDialog
{
  Q_OBJECT

public:
    PTXUISaveDialog( PTXPatientData &pd, QWidget *parent = 0 );
    ~PTXUISaveDialog();
   
    PTXPatientData &data;
    
    QGridLayout *layout_Grid;
    QVBoxLayout *layout_Vertical;
    QHBoxLayout *layout_Horizontal;

    QLabel *label_ID;
    QLineEdit *edit_ID;
 
    QPushButton *button_FilePath;
    QComboBox *comboBox_FilePath;
    
    QLabel *label_Age;
    QSpinBox *spinBox_Age;
  
    QLabel *label_Diagnosis;
    QComboBox *comboBox_Diagnosis;
   
    QPushButton *button_Cancel;
    QPushButton *button_Continue;
    QPushButton *button_Done;

protected:
  
    protected slots:

    void BrowseOutputDirectory();
    void Cancel();
    void Continue();
    void Done();
   
private:

   void SavePatientData(); 

};

QT_END_NAMESPACE

#endif // UI_PTX_H
