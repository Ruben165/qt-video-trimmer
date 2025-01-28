#ifndef TRIMMER_H
#define TRIMMER_H

#include "ui_trimmer.h"
#include <QMediaPlayer>
#include <QGraphicsVideoItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QAudioOutput>

class Trimmer : public QDialog, private Ui::Trimmer
{
    Q_OBJECT

public:
    explicit Trimmer(const QString &filePath, QWidget *parent = nullptr);
private slots:
    void on_buttonBox_accepted();
    void on_pushButtonPause_clicked();
    void on_pushButtonPlay_clicked();
    void updateSliderPosition(); // Sync slider as video plays
    void setSliderRange();       // Set the slider's maximum when video is loaded
    void on_horizontalSlider_sliderMoved(int position); // Seek video
    void on_horizontalSlider_sliderPressed();
    void on_horizontalSlider_sliderReleased();
    void on_buttonBox_rejected();

private:
    QString videoFilePath;
    QMediaPlayer *mediaPlayer;
    QGraphicsVideoItem *videoItem;
    QGraphicsScene *scene;
    bool isSliderPressed= false;
    void updateCurrentTimeLabel();
    void setEndTimeLabel(qint64 duration);
protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // TRIMMER_H
