#ifndef MODIFICATOR_H
#define MODIFICATOR_H

#include <QByteArray>
#include <QFileInfo>
#include <QString>

class Modificator : public QObject {
    Q_OBJECT
public:
    explicit Modificator(QObject* parent = nullptr);

    bool RunModificator();

// setters
    void SetInputMask(const QString& input_mask);
    void SetInputDirectory(const QString& input_directory);
    void SetOutputPath(const QString& output_path);

    void SetModifier(const QByteArray& modifier);

    void SetDeleteInput(bool delete_input);
    void SetOverwriteOutput(bool overwrite_output);

signals:
    void UpdateProgress(int value, int max);
    void ModifyFile(const QString& name, bool success);
    void FinishModify(int complete, int max);

private:
    QString input_mask_;
    QString input_directory_;
    QString output_path_;

    QByteArray modifier_;

    bool delete_input_ = false;
    bool overwrite_output_ = false;

// methods
    QString GetOutputName(const QFileInfo& input_file);
    bool XORModificate(const QString& input_name, const QString& output_name);
};

#endif // MODIFICATOR_H
