#pragma once
#include "qtsingleapplication/qtsingleapplication.h"

//Copies qApp
class BMApplication;
#define bmApp (static_cast<BMApplication*>(QCoreApplication::instance()))

class BMApplication : public QtSingleApplication
{
    Q_OBJECT

public:
    explicit BMApplication(const QString& id, int argc, char**argv);
    ~BMApplication();
};
