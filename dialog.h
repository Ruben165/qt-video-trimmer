#ifndef DIALOG_H
#define DIALOG_H

#include "ui_dialog.h"

#include <QDialog>

class Dialog : public QDialog, private Ui::Dialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();
    QString downloadedFilePath;
private slots:
    void on_pushButtonDownload_clicked();
    void on_pushButtonTrim_clicked();
};
#endif // DIALOG_H
