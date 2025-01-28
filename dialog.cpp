#include "dialog.h"
#include <QProcess>
#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QMessageBox>
#include "trimmer.h"

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
}

Dialog::~Dialog() {}

void Dialog::on_pushButtonDownload_clicked()
{
    QString videoLink = lineEditLink->text();
    qDebug() << "Video Link:" << videoLink;

    // Define the YT-DLP command and arguments
    QStringList arguments;
    arguments << "-f" << "mp4";  // Force the download in MP4 format
    arguments << "-o" << "video.mp4";
    arguments << "--progress";  // Optionally, you can use this for more progress output
    arguments << videoLink;

    // Set up the QProcess to run the yt-dlp command
    QProcess *process = new QProcess(this);

    // Start the yt-dlp command with the link as an argument
    process->start("yt-dlp", arguments);

    // Connect the finished signal to handle completion
    connect(process, &QProcess::finished, this, [=](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus == QProcess::NormalExit) {
            qDebug() << "Download complete! Exit code:" << exitCode;
        } else {
            qDebug() << "Download failed! Exit code:" << exitCode;
        }
    });

    // Handle output for progress and file path extraction
    connect(process, &QProcess::readyReadStandardOutput, this, [=]() {
        QByteArray output = process->readAllStandardOutput();
        QString strOutput = QString::fromUtf8(output);

        // Log the full output to see the format
        qDebug() << "YT-DLP Output: " << strOutput;

        // Extract download progress
        QRegularExpression regexProgress("(\\d+\\.\\d+)%");
        QRegularExpressionMatch matchProgress = regexProgress.match(strOutput);

        if (matchProgress.hasMatch()) {
            double progress = matchProgress.captured(1).toDouble();
            progressBar->setValue(static_cast<int>(progress));
            qDebug() << "Download Progress: " << progress << "%";
        }

        // Extract the downloaded file path
        QRegularExpression regexPath("Destination: (.*\\.\\w+)");  // Matches "Destination: path/to/file.ext"
        QRegularExpressionMatch matchPath = regexPath.match(strOutput);

        if (matchPath.hasMatch()) {
            downloadedFilePath = matchPath.captured(1).trimmed();  // Save the file path
            qDebug() << "Downloaded File Path: " << downloadedFilePath;
        }
    });

    // Handle errors and show a QMessageBox if necessary
    connect(process, &QProcess::readyReadStandardError, this, [=]() {
        QByteArray error = process->readAllStandardError();
        QString errorMessage = QString::fromUtf8(error);
        qDebug() << "YT-DLP Error: " << errorMessage;

        // Check for specific error patterns
        if (errorMessage.contains("ERROR:")) {  // Adjust based on yt-dlp error output format
            QMessageBox::critical(this, "Download Error",
                                  "There's an error, check the link again");
        }
    });
}


void Dialog::on_pushButtonTrim_clicked()
{
    if (downloadedFilePath.isEmpty()) {
        qDebug() << "No video has been downloaded yet!";
        return;
    }

    // Pass the file path to the Trimmer form
    Trimmer *trimmerForm = new Trimmer(downloadedFilePath, this);

    // Show the Trimmer form as a modal dialog
    trimmerForm->exec();
}

