#include "PTXUISaveDialog.h"

#include <QFileDialog>
#include <QFileInfo>


PTXUISaveDialog::PTXUISaveDialog(PTXPatientData &pd, QWidget *parent) 
    : data(pd), QDialog(parent)
{
   
    layout_Grid = new QGridLayout();
    layout_Vertical = new QVBoxLayout();
    layout_Horizontal = new QHBoxLayout();

    this->setLayout( layout_Vertical );
    layout_Vertical->addLayout( layout_Grid );
    layout_Vertical->addStretch(0);
    layout_Vertical->addLayout( layout_Horizontal );

    label_ID = new QLabel( tr("Patient ID") );
    edit_ID  = new QLineEdit();
    edit_ID->setText( QString::fromStdString( data.id ) );
    label_ID->setBuddy(edit_ID);

    layout_Grid->addWidget( label_ID, 0, 0, 1, 1, Qt::AlignTop);
    layout_Grid->addWidget( edit_ID , 0, 1, 1, 1, Qt::AlignTop);
   

    label_Age   = new QLabel(tr("Age") );
    spinBox_Age = new QSpinBox();
    spinBox_Age->setMinimum( 1 );
    spinBox_Age->setMaximum( 140 );
    spinBox_Age->setValue( data.age );
    label_Age->setBuddy( spinBox_Age );
    
    layout_Grid->addWidget( label_Age,   1, 0, 1, 1, Qt::AlignTop);
    layout_Grid->addWidget( spinBox_Age, 1, 1, 1, 1, Qt::AlignTop);

    label_Diagnosis= new QLabel( "Diagnosis" );
    comboBox_Diagnosis= new QComboBox();
    comboBox_Diagnosis->addItem( "No-PTX" );
    comboBox_Diagnosis->addItem( "Partial-PTX" );
    comboBox_Diagnosis->addItem( "Full-PTX" );
    label_Diagnosis->setBuddy( comboBox_Diagnosis);
    comboBox_Diagnosis->setCurrentText( QString::fromStdString(data.diagnosis) );
    
    layout_Grid->addWidget( comboBox_Diagnosis, 2, 1, 1, 1, Qt::AlignTop);
    layout_Grid->addWidget( label_Diagnosis,     2, 0, 1, 1, Qt::AlignTop);

    button_FilePath= new QPushButton( "Save Path" );
    comboBox_FilePath = new QComboBox();
    
    QString path = QString::fromStdString( data.filepath );
    QFileInfo pathInfo( path );
    if( pathInfo.exists() && pathInfo.isDir() )
      {
      comboBox_FilePath->addItem( path );
      }
    else
      {
      path = QDir::homePath();
      data.filepath = path.toStdString();
      comboBox_FilePath->addItem( path );
      }
   
    layout_Grid->addWidget( button_FilePath,    3, 0, 1, 1, Qt::AlignTop);
    layout_Grid->addWidget( comboBox_FilePath, 3, 1, 1, 1, Qt::AlignTop);
    

    button_Cancel   = new QPushButton("Cancel");
    button_Continue = new QPushButton("Continue");
    button_Done     = new QPushButton("Done");
    
    layout_Horizontal->addWidget( button_Cancel   );  
    layout_Horizontal->addWidget( button_Continue );  
    layout_Horizontal->addWidget( button_Done     );  
    

    connect( button_FilePath,
             SIGNAL( clicked() ), this, SLOT( BrowseOutputDirectory() ) );
    connect( button_Cancel,
             SIGNAL( clicked() ), this, SLOT( Cancel() ) );
    connect( button_Continue,
             SIGNAL( clicked() ), this, SLOT( Continue() ) );
    connect( button_Done,
             SIGNAL( clicked() ), this, SLOT( Done() ) );

}


PTXUISaveDialog::~PTXUISaveDialog()
  {
  }


void PTXUISaveDialog::BrowseOutputDirectory()
  {
  QString outputFolder = QFileDialog::getExistingDirectory( this, 
                                      "Choose directory to save the captured images", 
                                      QString::fromStdString( data.filepath ) 
                                                          );
  QFileInfo pathInfo( outputFolder);
  if(!( pathInfo.exists() && pathInfo.isDir() ) )
    {
    outputFolder = QDir::homePath();
    comboBox_FilePath->addItem( outputFolder );
    }
  comboBox_FilePath->insertItem( 0, outputFolder );
  comboBox_FilePath->setCurrentIndex( 0 );
  data.filepath = outputFolder.toStdString();
  }

void PTXUISaveDialog::Cancel()
  {
  done(0);
  }

void PTXUISaveDialog::Continue()
  {
  SavePatientData();
  done(1);
  }

void PTXUISaveDialog::Done()
  {
  SavePatientData();
  done(2);
  }

void PTXUISaveDialog::SavePatientData()
  {
  data.age = spinBox_Age->value();
  data.id =  edit_ID->text().toStdString();
  data.diagnosis =  comboBox_Diagnosis->currentText().toStdString();
  }
