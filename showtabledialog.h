#ifndef SHOWTABLEDIALOG_H
#define SHOWTABLEDIALOG_H

#include "ui_showtabledialog.h"

class ShowTableDialog : public QDialog, private Ui::ShowTableDialog
{
    Q_OBJECT

public:
    explicit ShowTableDialog(QWidget *parent = 0);
    void tableHeaderItemSetting(const QStringList &headerList);
    void tableCellContentSetting(int r, int c, const QString &text);
    int getTableRowCount() const;
    int getTableColumnCount() const;
    void setTableRowCount(int row);
    void setTableColumnCount(int col);
    void tableCellFormatSetting();

private slots:
    void on_tableExportButton_clicked();

    void on_buttonTxt_clicked();

protected:

private:

};

#endif // SHOWTABLEDIALOG_H
