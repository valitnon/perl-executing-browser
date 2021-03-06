/*
 Perl Executing Browser

 This program is free software;
 you can redistribute it and/or modify it under the terms of the
 GNU Lesser General Public License,
 as published by the Free Software Foundation;
 either version 3 of the License, or (at your option) any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.
 Dimitar D. Mitov, 2013 - 2019
 Valcho Nedelchev, 2014 - 2016
 https://github.com/ddmitov/perl-executing-browser
*/

#include <QObject>
#include <QTcpSocket>

#ifndef PORT_SCANNER_H
#define PORT_SCANNER_H

// ==============================
// PORT SCANNER CLASS DEFINITION:
// ==============================
class QPortScanner : public QObject
{
    Q_OBJECT

public:
    explicit QPortScanner(qint16 startPort, qint16 endPort);
    qint16 port;
    QString portScannerError;
};

#endif // PORT_SCANNER_H
