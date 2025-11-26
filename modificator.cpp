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
// методом добавления в конец имени файласкобочек с индексом, например - (2)
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

    if (!file_list.size()) {
        emit FinishModify(0, 0);
        return false;
    }

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
        emit UpdateProgress(complete_files, file_list.size());
    }
    emit FinishModify(succed_files, file_list.size());
    return true;
}
// Читает файл ,применяет XOR операцию, записывает в файл
bool Modificator::XORModificate(const QString& input_name, const QString& output_name) {
    QFile in(input_name);
    if (!in.open(QIODeviceBase::ReadOnly)) {
        return false;
    }

    QFile out(output_name);
    if (!out.open(QIODeviceBase::WriteOnly)) {
        in.close();
        return false;
    }

    const int buf_size = modifier_.size() * 1024; // 8 kilobyte
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
            return false;
            break;
        }
    }
    in.close();
    out.close();
    return true;
}
