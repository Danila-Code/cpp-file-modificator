#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "modificator.h"

#include <QLineEdit>
#include <QMainWindow>
#include <QProgressBar>
#include <QTimer>
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pb_modificate_clicked();

    void on_pb_browse_input_clicked();

    void on_pb_browse_output_clicked();

    void on_cbx_timer_toggled(bool checked);

    void update_progress(int value);

    void file_modified(const QString& file_name, bool success);

    void finish_modify(int succed_files, int total);

    void on_time_out();

    void error_process(const QString& msg);

signals:
    void operate();

private:
    Ui::MainWindow *ui;

    QThread* modificator_thread_;
    Modificator* file_modificator_;
    QTimer timer_;

    bool timer_on_ = false;
    bool is_working_ = false;
    uint32_t timer_period_ = 5;

    void SetUpModificator();
    void SetModificatorParams();
    void SetInitParams();
    bool CheckValidity();
    QString GetCurrentPath(QLineEdit* const le) const;
    void RunModificator();
    void StopTimer();
};
#endif // MAINWINDOW_H
