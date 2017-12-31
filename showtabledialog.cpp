#include <QAbstractItemView>
#include <QStandardPaths>
#include <QMessageBox>
#include <QTextStream>
#include <QFileDialog>
#include <QAxObject>
#include <QDebug>
#include "showtabledialog.h"

ShowTableDialog::ShowTableDialog(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);
    setWindowIcon(QIcon(":/new/prefix1/images/goldstar.png"));
    setWindowTitle(QStringLiteral("温度与对应亮度的报表"));
    dataTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    dataTableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    dataTableWidget->horizontalHeader()->setVisible(false);
    dataTableWidget->verticalHeader()->setVisible(false);
    dataTableWidget->resizeColumnsToContents();
    dataTableWidget->resizeRowsToContents();
}

int ShowTableDialog::getTableRowCount() const
{
    return dataTableWidget->rowCount();
}

int ShowTableDialog::getTableColumnCount() const
{
    return dataTableWidget->columnCount();
}

void ShowTableDialog::setTableRowCount(int row)
{
    int rowCnt = row;
    dataTableWidget->setRowCount(rowCnt);
}

void ShowTableDialog::setTableColumnCount(int col)
{
    int colCnt = col;
    dataTableWidget->setColumnCount(colCnt);
}

void ShowTableDialog::tableHeaderItemSetting(const QStringList &headerList)
{
    //设置表格列头的内容
    int colCnt = headerList.size();
    dataTableWidget->setColumnCount(colCnt);
    for(int i = 0; i < colCnt; i++){
        dataTableWidget->setItem(0, i, new QTableWidgetItem(headerList.at(i)));
    }
}

void ShowTableDialog::tableCellContentSetting(int r, int c, const QString &text)
{
    //设置表格的单元格内容
    dataTableWidget->setItem(r, c, new QTableWidgetItem(text));
}

void ShowTableDialog::tableCellFormatSetting()
{
    //对单元格的格式进行设置
    QFont font = dataTableWidget->verticalHeader()->font();
    font.setBold(true);
    dataTableWidget->verticalHeader()->setFont(font);
    int row = dataTableWidget->rowCount();
    int col = dataTableWidget->columnCount();
    for(int i = 1; i < row; ++i){
        for(int j = 0; j < col; ++j){
            QTableWidgetItem item;
            item.setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            item.setBackgroundColor(QColor(0, 191, 95));
            item.setTextColor(QColor(200, 111, 100));
            dataTableWidget->setItem(i, j, &item);
        }
    }
}

void ShowTableDialog::on_tableExportButton_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, "SaveAs", "untitle", "Microsoft Excel 2013(*.xlsx)");
    if(!filePath.isEmpty()){
        QAxObject *excel = new QAxObject(this);
        excel->setControl("Excel.Application");                        //连接EXCEL控件
        excel->dynamicCall("SetVisible(bool Visible)", false);         //不显示窗体
        excel->setProperty("DisplayAlerts", false);
        //不显示任何警告信息，如果为TRUE，那么在关闭时会出现‘文件已修改，是否保存’的提示
        QAxObject *workbooks = excel->querySubObject("WorkBooks");     //获取工作簿集合
        workbooks->dynamicCall("Add");                                 //新建一个工作簿
        QAxObject *workbook = excel->querySubObject("ActiveWorkBook"); //获取当前工作簿
        QAxObject *worksheets = workbook->querySubObject("Sheets");    //获取工作簿集合
        QAxObject *worksheet = worksheets->querySubObject("Item(int)", 1);
        //获取工作簿集合的工作表1，即sheet1
        int rowCount = dataTableWidget->rowCount();
        int columnCount = dataTableWidget->columnCount();
        //获取表格内容的行与列，作为EXCEL的行列
        for(int i = 1; i < rowCount + 1; ++i){
            for(int j = 1; j < columnCount + 1; ++j){
                //获取单元格,设置单元格的值
                QAxObject *Range = worksheet->querySubObject("Cells(int,int)", i, j);
                Range->dynamicCall("SetValue(const QString &)", dataTableWidget->item(i-1,j-1)->text());
            }
        }
        workbook->dynamicCall("SaveAs(const QString &)", QDir::toNativeSeparators(filePath));
        //保存至filepath，注意一定要用QDir::toNativeSeparators，将路径中的"/"转换为"\"，不然一定保存不了
        if(excel != NULL){
            excel->dynamicCall("Quit()");    //关闭工作簿
            delete excel;
            excel = NULL;
        }
        //提示完成数据的导出
        QMessageBox::information(this,QStringLiteral("提示"), QStringLiteral("保存成功，路径为：") + QString("%1").arg(filePath));
    }
}

void ShowTableDialog::on_buttonTxt_clicked()
{
    //将报表内容导出至相应的路径下的文本中
    if(QMessageBox::question(this, tr("WARN"), QStringLiteral("确定要把表格中数据导出吗？"), QMessageBox::Ok|QMessageBox::Cancel) == QMessageBox::Ok){
        QString fileName = QFileDialog::getSaveFileName(this, "Save as", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), QObject::tr("text File(*.csv)"));
        if (!fileName.isEmpty()){
            qDebug()<<"FileName Route:"<<fileName;
            QFile f(fileName);
            if(f.exists()){
                QMessageBox::warning(this, "WARNING", "This file exists!",QMessageBox::Ok);
            }
            qDebug()<<"000";
            if(!f.open(QIODevice::WriteOnly|QIODevice::Text)){
                QMessageBox::critical(this, "NOTICE", "Cannot create file");
            }
            qDebug()<<"111";
            QTextStream out(&f);
            int rowC = dataTableWidget->rowCount();
            int colC = dataTableWidget->columnCount();
            for(int i = 0; i < rowC; i++ ){
                for(int j = 0; j < colC; j++){
                    out<<dataTableWidget->item(i, j)->text()<<",  ";
                }
                out<<endl;
            }
        }
    }
}
