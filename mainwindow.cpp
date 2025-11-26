#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpressionValidator>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("File modificator");

    QRegularExpression rx("[A-F0-9]{16}");
    ui->le_modifier->setValidator(new QRegularExpressionValidator(rx, this));

    file_modificator_ = new Modificator;
    file_modificator_->moveToThread(&modificator_thread_);

    connect(file_modificator_, &Modificator::UpdateProgress, this, &MainWindow::update_progress);
    connect(file_modificator_, &Modificator::ModifyFile, this, &MainWindow::file_modified);
    connect(file_modificator_, &Modificator::FinishModify, this, &MainWindow::finish_modify);

    SetInitParams();
}

MainWindow::~MainWindow()
{
    modificator_thread_.quit();
    modificator_thread_.wait();
    delete file_modificator_;
    delete ui;
}

// установка начальных параметров
void MainWindow::SetInitParams() {
    ui->le_input_dir->setText(QDir::currentPath());
    ui->le_output_path->setText(QDir::currentPath());
    ui->le_input_mask->setText("*.txt");
    ui->le_modifier->setText("D2A1B3A4D2A1B3A4");
    ui->sbx_timer_period->setValue(timer_period_);
    ui->sbx_timer_period->setEnabled(false);
    ui->cbx_timer->setChecked(timer_on_);
    ui->pb_progress->setAlignment(Qt::AlignCenter);
}

// обновление progressbar
void MainWindow::update_progress(int value, int max) {
    ui->pb_progress->setValue(value * 100 / max);
}

// обновление списка обработанных файлов
void MainWindow::file_modified(const QString& file_name, bool success) {
    QString status = success ? "ОК" : "Error";
    ui->lst_status->addItem(file_name + " - " + status);
}
// завершение обработки файлов
void MainWindow::finish_modify(int succed_files, int total) {
    ui->pb_progress->setTextVisible(true);
    QString status = QString("Всего обработано файлов: %1 из %2").arg(succed_files).arg(total);
    ui->pb_progress->setFormat(status);

    ui->pb_modificate->setEnabled(true);
    if (timer_on_) {
        QTimer::singleShot(timer_period_ * 1000, file_modificator_, [this](){
            file_modificator_->RunModificator();
        });
    }
}

// запуск обработки файлов
void MainWindow::on_pb_modificate_clicked() {
    if (working_with_timer_) {
        working_with_timer_ = false;
        timer_on_ = false;
        ui->cbx_timer->setChecked(false);
        ui->sbx_timer_period->setEnabled(false);
        ui->pb_modificate->setText("Модифицировать файлы");
        ui->pb_modificate->setEnabled(false);
        return;
    }

    if (!CheckValidity()) {
        return;
    }
    SetModificatorParams();

    if (timer_on_) {
        working_with_timer_ = true;
        ui->pb_modificate->setText("Остановить таймер");
    } else {
        ui->pb_modificate->setEnabled(false);
    }

    modificator_thread_.start();
    QTimer::singleShot(0, file_modificator_, [this](){
        file_modificator_->RunModificator();
    });
}

void MainWindow::on_pb_browse_input_clicked() {
    QString current_path = ui->le_input_dir->text().isEmpty()
                               ? QDir::currentPath()
                               : ui->le_input_dir->text();
    ui->le_input_dir->setText(QFileDialog::getExistingDirectory(this,
                                           QString("Папка исходных файлов"),
                                           current_path,
                                           QFileDialog::ShowDirsOnly
                                           | QFileDialog::DontResolveSymlinks));
}

void MainWindow::on_pb_browse_output_clicked() {
    QString current_path = ui->le_output_path->text().isEmpty()
                               ? QDir::currentPath()
                               : ui->le_output_path->text();
    ui->le_output_path->setText(QFileDialog::getExistingDirectory(this,
                                           QString("Папка выходных файлов"),
                                           current_path,
                                           QFileDialog::ShowDirsOnly
                                           | QFileDialog::DontResolveSymlinks));
}
// проверяет валидность параметров
bool MainWindow::CheckValidity() {
    if (ui->le_input_dir->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "задайте папку входных файлов");
        return false;
    }
    if (ui->le_output_path->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "задайте папку для выходных файлов");
        return false;
    }
    if (ui->le_modifier->text().length() != 16) {
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
}
