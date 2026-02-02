#include "modificator.h"

#include <QDir>
#include <QFile>

namespace {
// применяет к буферу операцию XOR
void ApplyXORModifier(QByteArray& buffer, const QByteArray& modifier) {
    for (int i = 0; i < buffer.size(); ++i) {
        buffer[i] = buffer[i] ^ modifier[i % modifier.size()];
    }
}
// возвращает шаг обновления, который не должен быть меньше, чем один процент от size
int GetUpdateStep(qsizetype size) {
    const int max_step_count = 100;
    int step = size / max_step_count;
    return size % max_step_count == 0 ? step : ++step;
}
}  // namespace

Modificator::Modificator(QObject* parent) : QObject(parent) {
}

// setters methods
void Modificator::SetInputMask(const QString& input_mask) {
    input_mask_ = input_mask;
}

void Modificator::SetInputDirectory(const QString& input_directory) {
    input_directory_ = input_directory;
}

void Modificator::SetOutputPath(const QString& output_path) {
    output_path_ = output_path;
}

void Modificator::SetModifier(const QByteArray& modifier) {
    modifier_ = modifier;
}

void Modificator::SetDeleteInput(bool delete_input) {
    delete_input_ = delete_input;
}

void Modificator::SetOverwriteOutput(bool overwrite_output) {
    overwrite_output_ = overwrite_output;
}
// генерирует новое имя выходного файла (если такое имя уже есть)
// методом добавления в конец имени файла скобочек с индексом, например - file(2).txt
QString Modificator::GetOutputName(const QFileInfo& input_file) {
    QString output_file = output_path_ + "/" + input_file.fileName();
    if (!overwrite_output_) {
        if (QFile::exists(output_file)) {
            int count = 1;
            QString extension = input_file.suffix().isEmpty() ? "" : "." + input_file.suffix();
            QString base_name = output_path_ + "/" + input_file.baseName();
            do {
                output_file = QString("%1(%2)%3")
                                  .arg(base_name)
                                  .arg(count)
                                  .arg(extension);
                ++count;
            } while (QFile::exists(output_file));
        }
    }
    return output_file;
}
// ищет подходящие файлы и запускает их модификацию
bool Modificator::RunModificator() {
    QDir input_dir(input_directory_);

    input_dir.setFilter(QDir::Files | QDir::NoSymLinks);
    input_mask_ = input_mask_.isEmpty() ? "*" : input_mask_;
    input_dir.setNameFilters(QStringList(input_mask_));

    QFileInfoList file_list = input_dir.entryInfoList();
    const qsizetype size = file_list.size();

    if (!size) {
        emit FinishModify(0, 0);
        return false;
    }

    const int update_step = GetUpdateStep(size);

    int complete_files = 0;
    int succed_files = 0;
    for (const auto& file : file_list) {
        QString input_file = file.absoluteFilePath();
        QString output_file = GetOutputName(file);

        bool success = XORModificate(input_file, output_file);
        succed_files = !success ? succed_files : ++succed_files;
        if (success && delete_input_) {
                QFile::remove(input_file);
        }
        emit ModifyFile(file.fileName(), success);

        ++complete_files;
        if (complete_files % update_step == 0) {
            emit UpdateProgress(complete_files * 100 / size);
        }
    }
    emit FinishModify(succed_files, size);
    return true;
}
// Читает файл ,применяет XOR операцию, записывает в файл
bool Modificator::XORModificate(const QString& input_name, const QString& output_name) {
    QFile in(input_name);
    if (!in.open(QIODeviceBase::ReadOnly)) {
        emit ErrorMessage(QString("Не открывается входной файл: %1").arg(in.fileName()));
        return false;
    }

    QFile out(output_name);
    if (!out.open(QIODeviceBase::WriteOnly)) {
        in.close();
        emit ErrorMessage(QString("Не открывается выходной файл: %1").arg(out.fileName()));
        return false;
    }

    QByteArray buffer;

    while (!in.atEnd()) {
        buffer = in.read(buf_size);
        if (buffer.isEmpty()) {
            break;
        }

        ApplyXORModifier(buffer, modifier_);

        if (out.write(buffer) != buffer.size()) {
            in.close();
            out.close();
            emit ErrorMessage(QString("Ошибка записа в файл: %1").arg(out.fileName()));
            return false;
        }
    }
    in.close();
    out.close();
    return true;
}
