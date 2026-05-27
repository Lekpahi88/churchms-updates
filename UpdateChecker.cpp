#include "UpdateChecker.h"
#include <QNetworkRequest>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QPushButton>

static const QString UPDATE_URL =
    "https://raw.githubusercontent.com/Lekpahi88/churchms-updates/main/version.txt";

UpdateChecker& UpdateChecker::instance() {
    static UpdateChecker inst;
    return inst;
}

UpdateChecker::UpdateChecker(QObject* parent) : QObject(parent) {
    m_nam = new QNetworkAccessManager(this);
}

void UpdateChecker::checkForUpdates(bool silent) {
    QUrl url(UPDATE_URL);
    QNetworkRequest req{url};  // braces fix vexing parse
    req.setTransferTimeout(5000);

    QNetworkReply* reply = m_nam->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, silent]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Update check failed:" << reply->errorString();
            return;
        }

        QString content = QString::fromUtf8(reply->readAll()).trimmed();
        QStringList lines = content.split('\n');
        if (lines.isEmpty()) return;

        QString latestVersion = lines[0].trimmed();
        QString downloadUrl   = lines.size() > 1 ? lines[1].trimmed() : "";
        QString releaseNotes  = lines.size() > 2 ? lines[2].trimmed() : "";

        if (latestVersion <= currentVersion()) {
            if (!silent)
                QMessageBox::information(nullptr, "No Updates",
                    "ChurchMS v" + currentVersion() + " is up to date.");
            return;
        }

        emit updateAvailable(latestVersion, downloadUrl, releaseNotes);

        QMessageBox msgBox;
        msgBox.setWindowTitle("Update Available");
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText(
            "<b>ChurchMS v" + latestVersion + " is available!</b><br><br>"
            "You are running v" + currentVersion() + ".<br><br>"
            + (releaseNotes.isEmpty() ? "" :
               "<b>What's new:</b><br>" + releaseNotes.toHtmlEscaped() + "<br><br>")
            + "Download the update now?");
        msgBox.setStyleSheet(
            "QMessageBox{background:#FAF7F0;}"
            "QLabel{color:#1B3A6B;font-size:13px;min-width:350px;}");

        QPushButton* dlBtn = msgBox.addButton(
            " Download Update ", QMessageBox::AcceptRole);
        dlBtn->setStyleSheet(
            "background:#B8962E;color:white;border-radius:5px;"
            "padding:6px 16px;font-weight:bold;");
        QPushButton* laterBtn = msgBox.addButton(
            " Remind Me Later ", QMessageBox::RejectRole);
        laterBtn->setStyleSheet(
            "background:#888;color:white;border-radius:5px;padding:6px 16px;");
        Q_UNUSED(laterBtn)

        msgBox.exec();

        if (msgBox.clickedButton() == dlBtn && !downloadUrl.isEmpty())
            QDesktopServices::openUrl(QUrl(downloadUrl));
    });
}
