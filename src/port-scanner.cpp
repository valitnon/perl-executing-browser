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
 Dimitar D. Mitov, 2013 - 2017
 Valcho Nedelchev, 2014 - 2016
 https://github.com/ddmitov/perl-executing-browser
*/

#include <QTcpSocket>
#include "port-scanner.h"

// ==============================
// PORT SCANNER CONSTRUCTOR:
// ==============================
QPortScanner::QPortScanner(qint16 startPort, qint16 endPort)
    : QObject(0)
{
    // Google Chrome unsafe ports:
    QHash<qint16, QString> unsafePorts;
    unsafePorts.insert(2049, "nfs");
    unsafePorts.insert(3659, "apple-sasl / PasswordServer");
    unsafePorts.insert(4045, "lockd");
    unsafePorts.insert(6000, "X11");
    unsafePorts.insert(6665, "Alternate IRC [Apple addition]");
    unsafePorts.insert(6666, "Alternate IRC [Apple addition]");
    unsafePorts.insert(6667, "Standard IRC [Apple addition]");
    unsafePorts.insert(6668, "Alternate IRC [Apple addition]");
    unsafePorts.insert(6669, "Alternate IRC [Apple addition]");

    if (startPort <= 1024) {
        startPort = 1025;
    }

    if (endPort <= 1024) {
        endPort = 1025;
    }

    QList<qint16> safePorts;
    for (quint16 testedPort = startPort;
         testedPort <= endPort;
         testedPort++) {
        if (!unsafePorts.contains(testedPort)) {
            safePorts.append(testedPort);
        }
    }

    QTcpSocket *socket = new QTcpSocket();
    foreach (qint16 safePort, safePorts) {
        socket->bind(safePort, QAbstractSocket::DontShareAddress);
        qint16 localPort = socket->localPort();
        socket->close();
        socket->deleteLater();

        if (localPort == safePort) {
            port = safePort;
            break;
        }
    }
}
