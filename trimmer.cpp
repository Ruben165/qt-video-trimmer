#include "trimmer.h"
#include <QCloseEvent>
#include <QTimer>
#include <QTime>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QRegularExpression>

QString ffmpeg_path() {
    // Specify the full path to where the ffmpeg.exe is located in your system
    return "./ffmpeg-master-latest-win64-gpl-shared/bin/ffmpeg.exe";
}

Trimmer::Trimmer(const QString &filePath, QWidget *parent)
    : QDialog(parent), videoFilePath(filePath)
{
    setupUi(this);

    qDebug() << "Trimmer received file path: " << videoFilePath;
    lineEditName->setText("Trimmed_Video");

    // 1. Create QMediaPlayer to load and play the video
    mediaPlayer = new QMediaPlayer(this);

    // 2. Set up audio output for sound
    QAudioOutput *audioOutput = new QAudioOutput(this);
    mediaPlayer->setAudioOutput(audioOutput);

    // 3. Create QGraphicsVideoItem to display the video
    videoItem = new QGraphicsVideoItem();

    // 4. Set up a QGraphicsScene and attach the QGraphicsVideoItem
    scene = new QGraphicsScene(this);
    scene->addItem(videoItem);

    // 5. Set the scene to the QGraphicsView (from your UI)
    graphicsView->setScene(scene);

    // 6. Set the QGraphicsVideoItem as the output for the media player
    mediaPlayer->setVideoOutput(videoItem);

    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, [=](qint64 duration) {
        qDebug() << "Duration changed: " << duration / 1000 << " seconds";
        setEndTimeLabel(duration);
        lineEditStart->setText("00:00");
    });

    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, &Trimmer::updateSliderPosition);
    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, &Trimmer::setSliderRange);
    connect(horizontalSlider, &QSlider::sliderMoved, this, &Trimmer::on_horizontalSlider_sliderMoved);
    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, &Trimmer::setEndTimeLabel);

    // 8. Load the video file (you can try using one of these)
    mediaPlayer->setSource(QUrl::fromLocalFile(videoFilePath));

    mediaPlayer->play();

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Trimmer::updateCurrentTimeLabel);
    timer->start(100); // Update every 100 ms
}

void Trimmer::updateSliderPosition()
{
    if (!isSliderPressed) { // Only update if the slider is not being dragged
        horizontalSlider->setValue(static_cast<int>(mediaPlayer->position() / 1000));
    }
}

void Trimmer::closeEvent(QCloseEvent *event)
{
    mediaPlayer->stop();
    qDebug() << "Media playback stopped!";
}

void Trimmer::on_buttonBox_accepted()
{
    mediaPlayer->stop();
    // Get the start and end times from lineEditStart and lineEditEnd
    QString startTime = lineEditStart->text(); // Format: "mm:ss"
    QString endTime = lineEditEnd->text();     // Format: "mm:ss"
    QString fileName = lineEditName->text();   // Video file name

    // Convert the start and end times to seconds for FFmpeg
    QStringList startParts = startTime.split(":");
    int startSecs = startParts[0].toInt() * 60 + startParts[1].toInt();

    QStringList endParts = endTime.split(":");
    int endSecs = endParts[0].toInt() * 60 + endParts[1].toInt();

    // Get the video duration using FFmpeg
    QProcess *probeProcess = new QProcess(this);
    QStringList probeArguments;
    probeArguments << "-i" << videoFilePath; // Input video file

    // Start FFmpeg probe to get video duration
    probeProcess->start(ffmpeg_path(), probeArguments);
    probeProcess->waitForFinished();

    QByteArray probeOutput = probeProcess->readAllStandardError();
    QString probeOutputStr = QString::fromUtf8(probeOutput);

    // Extract the duration from FFmpeg output (in seconds)
    QRegularExpression durationRegex("Duration: (\\d+):(\\d+):(\\d+\\.\\d+)");
    QRegularExpressionMatch match = durationRegex.match(probeOutputStr);

    if (!match.hasMatch()) {
        QMessageBox::critical(this, "Error", "Unable to determine video duration.");
        return;
    }

    int videoHours = match.captured(1).toInt();
    int videoMinutes = match.captured(2).toInt();
    double videoSeconds = match.captured(3).toDouble();

    int videoDurationSecs = videoHours * 3600 + videoMinutes * 60 + static_cast<int>(videoSeconds);

    // Validation checks
    if (endSecs > videoDurationSecs) {
        QMessageBox::critical(this, "Error", "The end time is greater than the video duration.");
        return;
    }

    if (startSecs > endSecs) {
        QMessageBox::critical(this, "Error", "The start time cannot be greater than the end time.");
        return;
    }

    int durationSecs = endSecs - startSecs;

    // Get the directory of the current video file
    QFileInfo fileInfo(videoFilePath);
    QString outputFilePath = fileInfo.absolutePath() + "/" + fileName + ".mp4";

    // Ensure the directory exists
    QDir dir(fileInfo.absolutePath());
    if (!dir.exists()) {
        qDebug() << "Directory doesn't exist: " << fileInfo.absolutePath();
        return;
    }

    // Build FFmpeg command to trim the video
    QStringList arguments;
    arguments << "-i" << videoFilePath                // Input file
              << "-ss" << QString::number(startSecs)  // Start time (in seconds)
              << "-t" << QString::number(durationSecs) // Duration (in seconds)
              << "-c:v" << "libx264"                   // Video codec
              << "-c:a" << "aac"                       // Audio codec
              << "-strict" << "experimental"           // Allow experimental codecs
              << outputFilePath;                       // Output file path

    // Debugging output: print the command to be executed
    qDebug() << "Running FFmpeg command: ffmpeg " << arguments.join(" ");

    // Execute FFmpeg to trim the video
    QProcess *process = new QProcess(this);

    connect(process, &QProcess::readyReadStandardOutput, [process]() {
        qDebug() << "FFmpeg output: " << process->readAllStandardOutput();
    });

    // Start the process
    process->start(ffmpeg_path(), arguments);

    if (!process->waitForStarted()) {
        qDebug() << "Failed to start FFmpeg process. Error: " << process->errorString();
        return;
    }

    // Wait for FFmpeg to finish
    process->waitForFinished();

    // After execution, check if the process was successful
    if (process->exitStatus() == QProcess::NormalExit && process->exitCode() == 0) {
        // Success message
        QMessageBox::information(this, "Success", "Video trimmed successfully!");
        qDebug() << "Video saved successfully to: " << outputFilePath;
    } else {
        // Failure message
        QMessageBox::critical(this, "Error", "Failed to trim video.");
        qDebug() << "Failed to save video. Exit code: " << process->exitCode();
    }
}


void Trimmer::on_pushButtonPause_clicked()
{
    mediaPlayer->pause();
}


void Trimmer::on_pushButtonPlay_clicked()
{
    mediaPlayer->play();
}


void Trimmer::setSliderRange()
{
    int durationInSeconds = static_cast<int>(mediaPlayer->duration() / 1000); // Convert to seconds
    horizontalSlider->setMaximum(durationInSeconds);
    qDebug() << "Video duration: " << durationInSeconds << " seconds";
}

void Trimmer::updateCurrentTimeLabel()
{
    if (!isSliderPressed) { // Only update if the slider is not being dragged
        int currentTime = static_cast<int>(mediaPlayer->position() / 1000); // Get current position in seconds
        QString formattedTime = QTime(0, 0).addSecs(currentTime).toString("mm:ss"); // Format as 00:00
        currentTimeLabel->setText(formattedTime); // Update current time label
    }
}

void Trimmer::setEndTimeLabel(qint64 duration)
{
    int durationInSeconds = static_cast<int>(duration / 1000); // Convert to seconds
    QString formattedTime = QTime(0, 0).addSecs(durationInSeconds).toString("mm:ss"); // Format as 00:00
    lengthTimeLabel->setText(formattedTime); // Set the end time label once with video duration
    lineEditEnd->setText(formattedTime);
}

void Trimmer::on_horizontalSlider_sliderMoved(int position)
{
    mediaPlayer->setPosition(position * 1000); // Convert seconds to milliseconds
    int currentTime = position; // Get position in seconds
    QString formattedTime = QTime(0, 0).addSecs(currentTime).toString("mm:ss"); // Format as 00:00
    currentTimeLabel->setText(formattedTime); // Update the current time label immediately on slider move
}

void Trimmer::on_horizontalSlider_sliderPressed()
{
    isSliderPressed = true; // Mark that the slider is being dragged
    // qDebug() << "Slider pressed!";
}


void Trimmer::on_horizontalSlider_sliderReleased()
{
    isSliderPressed = false; // Mark that dragging is done
    mediaPlayer->setPosition(horizontalSlider->value() * 1000); // Seek to slider position
    updateCurrentTimeLabel(); // Ensure current time label is updated after slider release
}


void Trimmer::on_buttonBox_rejected()
{
    mediaPlayer->stop();
    qDebug() << "Media playback stopped!";
}
