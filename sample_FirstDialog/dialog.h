#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();

private slots:
    void on_pushButton_Clear_clicked();

    void on_checkBox_underline_clicked(bool checked);

    void on_checkBox_italic_clicked(bool checked);

    void on_checkBox_bold_clicked(bool checked);

    void do_FontColor();

private:
    Ui::Dialog *ui;
};//
#endif // DIALOG_H
