#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpressionValidator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setWindowTitle("File modificator");

    QRegularExpression rx("[A-F0-9]{16}");
    ui->le_modifier->setValidator(new QRegularExpressionValidator(rx, this));

    timer_.setSingleShot(true);
    connect(&timer_, &QTimer::timeout, this, &MainWindow::on_time_out);

    SetUpModificator();
    SetInitParams();
}

MainWindow::~MainWindow() {
    modificator_thread_->quit();
    modificator_thread_->wait();
    delete modificator_thread_;
    delete ui;
}
// задание потока модификатора и связей для модификатора файлов и его потока
void MainWindow::SetUpModificator() {
    modificator_thread_ = new QThread();
    file_modificator_ = new Modificator();
    file_modificator_->moveToThread(modificator_thread_);

    connect(file_modificator_, &Modificator::UpdateProgress, this, &MainWindow::update_progress);
    connect(file_modificator_, &Modificator::ModifyFile, this, &MainWindow::file_modified);
    connect(file_modificator_, &Modificator::FinishModify, this, &MainWindow::finish_modify);
    connect(file_modificator_, &Modificator::ErrorMessage, this, &MainWindow::error_process);

    connect(modificator_thread_, &QThread::finished, file_modificator_, &QObject::deleteLater);
    connect(this, &MainWindow::operate, file_modificator_, &Modificator::RunModificator);

    modificator_thread_->start();
}

// установка начальных параметров
void MainWindow::SetInitParams() {
    ui->le_input_dir->setText(QDir::currentPath());
    ui->le_output_path->setText(QDir::currentPath());
    ui->le_input_mask->setText("*.txt");
    ui->le_modifier->setText("D2A1B3A4D2A1B3A4");
    ui->sbx_timer_period->setValue(timer_period_);
    ui->sbx_timer_period->setEnabled(timer_on_);
    ui->lbl_timer_period->setEnabled(timer_on_);
    ui->cbx_timer->setChecked(timer_on_);
    ui->pb_progress->setStyleSheet("text-align: center;");
}

// обновление progressbar
void MainWindow::update_progress(int value) {
    ui->pb_progress->setValue(value);
}

// обновление списка обработанных файлов
void MainWindow::file_modified(const QString& file_name, bool success) {
    QString status = success ? "ОК" : "Error";
    ui->lst_status->addItem(file_name + " - " + status);
}
// завершение обработки файлов
void MainWindow::finish_modify(int succed_files, int total) {
    QString status = QString("Успешно обработано файлов: %1 из %2").arg(succed_files).arg(total);
    ui->lst_status->addItem(status);
    ui->pb_progress->setFormat("Завершено");
    ui->pb_progress->setValue(100);
    ui->pb_modificate->setEnabled(true);
    is_working_ = false;

    if (timer_on_) {
        timer_.start();
    }
}
// вызывает окно с ошибкой
void MainWindow::error_process(const QString& msg) {
    ui->pb_progress->setFormat("Error!");
    ui->pb_modificate->setEnabled(true);
    QMessageBox::critical(this, "Error", msg);
}

void MainWindow::on_time_out() {
    RunModificator();
}
// запуск обработки файлов
void MainWindow::on_pb_modificate_clicked() {
    if (timer_on_) {
        StopTimer();
        return;
    }

    if (!CheckValidity()) {
        return;
    }
    SetModificatorParams();

    if (ui->cbx_timer->checkState()) {
        timer_.setInterval(timer_period_ * 1000);
        timer_on_ = true;
        ui->pb_modificate->setText("Остановить таймер");
    }

    RunModificator();
}

void MainWindow::on_pb_browse_input_clicked() {
    QString dir = QFileDialog::getExistingDirectory(this,
                                                    QString("Папка исходных файлов"),
                                                    GetCurrentPath(ui->le_input_dir),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        ui->le_input_dir->setText(dir);
    }
}

void MainWindow::on_pb_browse_output_clicked() {
    QString dir = QFileDialog::getExistingDirectory(this,
                                                    QString("Папка выходных файлов"),
                                                    GetCurrentPath(ui->le_output_path),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        ui->le_output_path->setText(dir);
    }
}
// проверяет валидность параметров
bool MainWindow::CheckValidity() {
    QString dir = ui->le_input_dir->text();
    if (dir.isEmpty()) {
        QMessageBox::warning(this, "Error", "задайте папку входных файлов");
        return false;
    }
    if (!QDir(dir).exists()) {
        QMessageBox::warning(this, "Error", "входная папка не существует");
        return false;
    }

    dir = ui->le_output_path->text();
    if (dir.isEmpty()) {
        QMessageBox::warning(this, "Error", "задайте папку для выходных файлов");
        return false;
    }
    if (!QDir(dir).exists()) {
        QMessageBox::warning(this, "Error", "выходная папка не существует");
        return false;
    }

    if (qsizetype size = ui->le_modifier->text().size();
            !size || size != 16) {
        QMessageBox::warning(this, "Error", "модификатор должен состоять из 16 символов в HEX формате");
        return false;
    }
    return true;
}

// задание параметров модификатора файлов
void MainWindow::SetModificatorParams() {
    timer_on_ = ui->cbx_timer->checkState();
    timer_period_ = ui->sbx_timer_period->text().toInt();
    file_modificator_->SetInputDirectory(ui->le_input_dir->text());
    file_modificator_->SetOutputPath(ui->le_output_path->text());
    file_modificator_->SetInputMask(ui->le_input_mask->text());
    file_modificator_->SetDeleteInput(ui->cbx_delete->checkState());
    file_modificator_->SetOverwriteOutput(ui->cbx_overwrite->checkState());
    file_modificator_->SetModifier(QByteArray::fromHex(ui->le_modifier->text().toLatin1()));
}

void MainWindow::on_cbx_timer_toggled(bool checked) {
    ui->sbx_timer_period->setEnabled(checked);
    ui->lbl_timer_period->setEnabled(checked);
}
// если текст у lineedit пустой возвращает текущую папку программы
QString MainWindow::GetCurrentPath(QLineEdit* const le) const {
    return le->text().isEmpty() ? QDir::currentPath() : le->text();
}
// Запуск работы модификатора файлов
void MainWindow::RunModificator() {
    ui->lst_status->clear();
    ui->pb_progress->setValue(0);
    ui->pb_progress->setFormat("Старт процесса обработки ...");
    ui->pb_modificate->setEnabled(timer_on_);
    emit operate();
    is_working_ = true;
}
// Останавливает таймер
void MainWindow::StopTimer() {
    timer_.stop();
    timer_on_ = false;
    ui->cbx_timer->setChecked(false);
    ui->sbx_timer_period->setEnabled(false);
    ui->lbl_timer_period->setEnabled(false);
    ui->pb_modificate->setText("Модифицировать файлы");
    ui->pb_modificate->setEnabled(!is_working_);
}

