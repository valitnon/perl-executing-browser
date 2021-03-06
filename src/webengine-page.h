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

#ifndef PAGE_H
#define PAGE_H

#include <QFileDialog>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>
#include <QWebEnginePage>

#include "file-reader.h"
#include "script-handler.h"

// ==============================
// WEB PAGE CLASS DEFINITION:
// (QTWEBENGINE VERSION)
// ==============================
class QPage : public QWebEnginePage
{
    Q_OBJECT

signals:
    void pageLoadedSignal();
    void closeWindowSignal();

public slots:
    void qPageLoadedSlot(bool ok)
    {
        if (ok) {
            if (QPage::url().scheme() == "file") {
                // Inject all browser-specific Javascript:
                QFileReader *resourceReader =
                        new QFileReader(QString(":/peb.js"));
                QString pebJavaScript = resourceReader->fileContents;

                QPage::runJavaScript(pebJavaScript);

                QPage::runJavaScript(
                            QString("peb.getPageSettings()"),
                            [&](QVariant result){
                    qGetPageSettings(result);
                });
            }

            // Send signal to the html-viewing class that a page is loaded:
            emit pageLoadedSignal();

            // Log basic program information on the JavaScript console:
            QString applicationVersionMessage =
                    "console.log('Application version: " +
                    qApp->applicationVersion().toLatin1() + "');";
            QPage::runJavaScript(applicationVersionMessage);

            QString qtVersionMessage =
                    "console.log('Qt version: " +
                    QString(QT_VERSION_STR) + "');";
            QPage::runJavaScript(qtVersionMessage);

            QString perlInterpreterMessage =
                    "console.log('Perl interpreter: " +
                    qApp->property("perlInterpreter").toString() + "');";
            QPage::runJavaScript(perlInterpreterMessage);
        }
    }

    // ==============================
    // Page settings:
    // ==============================
    void qGetPageSettings(QVariant settingsJsResult) {
        QJsonDocument settingsJsonDocument =
                QJsonDocument::fromJson(settingsJsResult.toString().toUtf8());

        if (!settingsJsonDocument.isEmpty()) {
            QJsonObject settingsJsonObject = settingsJsonDocument.object();

            // Get auto-start scripts:
            QJsonArray autoStartScripts =
                    settingsJsonObject["autoStartScripts"].toArray();

            foreach (const QJsonValue &value, autoStartScripts) {
                QString autoStartScript = value.toString();
                qHandleScripts(autoStartScript);
            }

            // Get dialog and context menu labels:
            if (settingsJsonObject["okLabel"].toString().length() > 0) {
                okLabel = settingsJsonObject["okLabel"].toString();
            }

            if (settingsJsonObject["cancelLabel"].toString().length() > 0) {
                cancelLabel = settingsJsonObject["cancelLabel"].toString();
            }

            if (settingsJsonObject["yesLabel"].toString().length() > 0) {
                yesLabel = settingsJsonObject["yesLabel"].toString();
            }

            if (settingsJsonObject["noLabel"].toString().length() > 0) {
                noLabel = settingsJsonObject["noLabel"].toString();
            }

            if (settingsJsonObject["cutLabel"].toString().length() > 0) {
                qApp->setProperty("cutLabel",
                                  settingsJsonObject["cutLabel"].toString());
            }

            if (settingsJsonObject["copyLabel"].toString().length() > 0) {
                qApp->setProperty("copyLabel",
                                  settingsJsonObject["copyLabel"].toString());
            }

            if (settingsJsonObject["pasteLabel"].toString().length() > 0) {
                qApp->setProperty("pasteLabel",
                                  settingsJsonObject["pasteLabel"].toString());
            }

            if (settingsJsonObject["selectAllLabel"].toString().length() > 0) {
                qApp->setProperty(
                            "selectAllLabel",
                            settingsJsonObject["selectAllLabel"].toString());
            }
        }
    }

    // ==============================
    // Filesystem dialogs:
    // ==============================
    void qHandleDialogs(QString dialogObjectName)
    {
        QPage::runJavaScript(
                    QString("peb.getDialogSettings(" +
                            dialogObjectName + ")"),
                    [dialogObjectName, this](QVariant dialogSettings)
        {
            QJsonDocument dialogJsonDocument =
                    QJsonDocument::fromJson(
                        dialogSettings.toString().toUtf8());

            if (!dialogJsonDocument.isEmpty()) {
                QJsonObject dialogJsonObject = dialogJsonDocument.object();
                dialogJsonObject["id"] = dialogObjectName;
                qReadDialogSettings(dialogJsonObject);
            }
        });
    }

    void qReadDialogSettings(QJsonObject dialogJsonObject)
    {
        QString id = dialogJsonObject["id"].toString();

        QString type = dialogJsonObject["type"].toString();

        QFileDialog inodesDialog (qApp->activeWindow());
        inodesDialog.setWindowModality(Qt::WindowModal);
        inodesDialog.setViewMode(QFileDialog::Detail);

#ifdef Q_OS_WIN
        inodesDialog.setOption(QFileDialog::DontUseNativeDialog);
#endif

        if (type == "single-file") {
            inodesDialog.setFileMode(QFileDialog::AnyFile);
        }

        if (type == "multiple-files") {
            inodesDialog.setFileMode(QFileDialog::ExistingFiles);
        }

        if (type == "new-file-name") {
            inodesDialog.setAcceptMode(QFileDialog::AcceptSave);
        }

        if (type == "directory") {
            inodesDialog.setFileMode(QFileDialog::Directory);
        }

        QStringList selectedInodes;
        if (inodesDialog.exec()) {
            selectedInodes = inodesDialog.selectedFiles();
        }

        inodesDialog.close();
        inodesDialog.deleteLater();

        if (!selectedInodes.isEmpty()) {
            QString inodesFormatted;
            foreach (QString userSelectedInode, selectedInodes) {
                inodesFormatted.append(userSelectedInode);
                inodesFormatted.append(";");
            }
            inodesFormatted.replace(QRegularExpression(";$"), "");

            QString outputInsertionJavaScript =
                    id + ".receiverFunction('" + inodesFormatted + "'); null";

            QPage::runJavaScript(outputInsertionJavaScript);
        }
    }

    // ==============================
    // Perl scripts:
    // ==============================
    void qHandleScripts(QString scriptObjectName)
    {
        if (QPage::url().scheme() == "file") {
            QPage::runJavaScript(
                        QString("peb.getScriptSettings(" +
                                scriptObjectName + ")"),
                        [scriptObjectName, this](QVariant scriptSettings)
            {
                QJsonDocument scriptJsonDocument =
                        QJsonDocument::fromJson(
                            scriptSettings.toString().toUtf8());

                if (!scriptJsonDocument.isEmpty()) {
                    QJsonObject scriptJsonObject = scriptJsonDocument.object();
                    scriptJsonObject["id"] = scriptObjectName;
                    qScriptStartedCheck(scriptJsonObject);
                }
            });
        }
    }

    void qScriptStartedCheck(QJsonObject scriptJsonObject)
    {
        // Start the script if it is not yet started:
        if (!runningScripts.contains(scriptJsonObject["id"].toString())) {
            qStartScript(scriptJsonObject);
        }

        // Feed the script with data if it is already started:
        if (runningScripts.contains(scriptJsonObject["id"].toString())) {
            qFeedScript(scriptJsonObject);
        }
    }

    void qStartScript(QJsonObject scriptJsonObject)
    {
        QScriptHandler *scriptHandler = new QScriptHandler(scriptJsonObject);

        QObject::connect(scriptHandler,
                         SIGNAL(displayScriptOutputSignal(QString,
                                                          QString)),
                         this,
                         SLOT(qDisplayScriptOutputSlot(QString,
                                                       QString)));

        QObject::connect(scriptHandler,
                         SIGNAL(displayScriptErrorsSignal(QString)),
                         this,
                         SLOT(qDisplayScriptErrorsSlot(QString)));

        QObject::connect(scriptHandler, SIGNAL(scriptFinishedSignal(QString)),
                         this, SLOT(qScriptFinishedSlot(QString)));

        runningScripts.insert(scriptJsonObject["id"].toString(), scriptHandler);
    }

    void qFeedScript(QJsonObject scriptJsonObject)
    {
        QString scriptInput;
        if (scriptJsonObject["scriptInput"].toString().length() > 0) {
            scriptInput = scriptJsonObject["scriptInput"].toString();
        }

        if (scriptInput.length() > 0) {
            QScriptHandler *handler =
                    runningScripts.value(scriptJsonObject["id"].toString());
            if (handler->scriptProcess.isOpen()) {
                handler->scriptProcess.write(scriptInput.toUtf8());
                handler->scriptProcess.write(QString("\n").toLatin1());
            }
        }
    }

    void qDisplayScriptOutputSlot(QString id, QString output)
    {
        if (QPage::url().scheme() == "file") {
            QString outputInsertionJavaScript =
                    id + ".stdoutFunction('" + output + "'); null";

            QPage::runJavaScript(outputInsertionJavaScript);
        }
    }

    void qDisplayScriptErrorsSlot(QString errors)
    {
        if (QPage::url().scheme() == "file") {
            if (errors.length() > 0) {
                errors.replace("\"", "\\\"");
                errors.replace("\'", "\\'");
                errors.replace("\n", "\\n");
                errors.replace("\r", "");

                QString perlScriptErrorsMessage =
                        "console.log('" + errors + "'); null";

                QPage::runJavaScript(perlScriptErrorsMessage);
            }
        }
    }

    void qScriptFinishedSlot(QString scriptId)
    {
        runningScripts.remove(scriptId);

        if (closeRequested == true and runningScripts.isEmpty()) {
            emit closeWindowSignal();
        }
    }

    // ==============================
    // Page-closing routines:
    // ==============================
    void qStartWindowClosingSlot()
    {
        if (QPage::url().scheme() == "file") {
            QPage::runJavaScript(
                        QString("peb.checkUserInputBeforeClose()"),
                        [&](QVariant jsResult){
                qCloseWindow(jsResult);
            });
        } else {
            qCloseAllScriptsSlot();
        }
    }

    void qCloseWindow(QVariant jsResult)
    {
        bool jsCloseDecision = true;
        jsCloseDecision = jsResult.toBool();

        if (jsCloseDecision == true) {
            if (!runningScripts.isEmpty()){
                qCloseAllScriptsSlot();
            } else {
                emit closeWindowSignal();
            }
        }
    }

    void qCloseAllScriptsSlot()
    {
        closeRequested = true;

        if (!runningScripts.isEmpty()) {
            QHash<QString, QScriptHandler*>::iterator iterator;
            for (iterator = runningScripts.begin();
                 iterator != runningScripts.end();
                 ++iterator) {
                QScriptHandler *handler = iterator.value();

                if (handler->scriptProcess.isOpen()) {
#ifndef Q_OS_WIN
                    handler->scriptProcess.terminate();
#endif

#ifdef Q_OS_WIN
                    handler->scriptProcess.kill();
                    runningScripts.remove(iterator.key());
#endif
                }

                if (!handler->scriptProcess.isOpen()) {
                    runningScripts.remove(iterator.key());
                }
            }
        }

#ifndef Q_OS_WIN
        if (!runningScripts.isEmpty()) {
            int maximumTimeMilliseconds = 3000;
            QTimer::singleShot(maximumTimeMilliseconds,
                               this, SLOT(qScriptsTimeoutSlot()));
        }
#endif

        if (runningScripts.isEmpty()) {
            emit closeWindowSignal();
        }
    }

    void qScriptsTimeoutSlot()
    {
#ifndef Q_OS_WIN
        if (!runningScripts.isEmpty()) {
            QHash<QString, QScriptHandler*>::iterator iterator;
            for (iterator = runningScripts.begin();
                 iterator != runningScripts.end();
                 ++iterator) {
                QScriptHandler *handler = iterator.value();

                if (handler->scriptProcess.isOpen()) {
                    handler->scriptProcess.kill();
                }
            }
        }

        emit closeWindowSignal();
#endif
    }

protected:
    bool acceptNavigationRequest(const QUrl &url,
                                 QWebEnginePage::NavigationType type,
                                 bool isMainFrame) override;

    // ==============================
    // JavaScript Alert:
    // ==============================
    virtual void javaScriptAlert(const QUrl &url, const QString &msg)
    {
        Q_UNUSED(url);

        QMessageBox javaScriptAlertMessageBox (qApp->activeWindow());
        javaScriptAlertMessageBox.setWindowModality(Qt::WindowModal);
        javaScriptAlertMessageBox.setWindowTitle(title());
        javaScriptAlertMessageBox.setText(msg);
        javaScriptAlertMessageBox.setButtonText(QMessageBox::Ok, okLabel);
        javaScriptAlertMessageBox.setDefaultButton(QMessageBox::Ok);
        javaScriptAlertMessageBox.exec();
    }

    // ==============================
    // JavaScript Confirm:
    // ==============================
    virtual bool javaScriptConfirm(const QUrl &url, const QString &msg)
    {
        Q_UNUSED(url);

        QMessageBox javaScriptConfirmMessageBox (qApp->activeWindow());
        javaScriptConfirmMessageBox.setWindowModality(Qt::WindowModal);
        javaScriptConfirmMessageBox.setWindowTitle(title());
        javaScriptConfirmMessageBox.setText(msg);
        javaScriptConfirmMessageBox
                .setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        javaScriptConfirmMessageBox.setButtonText(QMessageBox::Yes, yesLabel);
        javaScriptConfirmMessageBox.setButtonText(QMessageBox::No, noLabel);
        return QMessageBox::Yes == javaScriptConfirmMessageBox.exec();
    }

    // ==============================
    // JavaScript Prompt:
    // ==============================
    virtual bool javaScriptPrompt(const QUrl &url, const QString &msg,
                                  const QString &defaultValue, QString *result)
    {
        Q_UNUSED(url);

        bool ok = false;

        QInputDialog dialog;
        dialog.setModal(true);
        dialog.setWindowTitle(title());
        dialog.setLabelText(msg);
        dialog.setInputMode(QInputDialog::TextInput);
        dialog.setTextValue(defaultValue);
        dialog.setOkButtonText(okLabel);
        dialog.setCancelButtonText(cancelLabel);

        if (dialog.exec() == QDialog::Accepted) {
            *result = dialog.textValue();
            ok = true;
            return ok;
        }

        return ok;
    }

private:
    QString okLabel;
    QString cancelLabel;
    QString yesLabel;
    QString noLabel;

    bool closeRequested;

public:
    QPage();
    QHash<QString, QScriptHandler*> runningScripts;
};

#endif // PAGE_H
