#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <algorithm>
#include <fstream>
#include "common.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QMouseEvent>
#include <QPainter>
#include <QAbstractItemView>
#include <QDateTime>
#include <QFileDialog>
#include <QHeaderView>
#include <QDockWidget>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QStatusBar>
#include <QTableWidgetItem>
#include <QVariantMap>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif
#include "drawwidget.h"
#pragma execution_character_set("utf-8")

namespace {
QString queryTypeToString(QueryType type) {
    switch (type) {
    case QueryType::Point:
        return QStringLiteral("点查询");
    case QueryType::Area:
        return QStringLiteral("区域查询");
    case QueryType::Adjacency:
        return QStringLiteral("邻近查询");
    case QueryType::Trajectory:
        return QStringLiteral("路径查询");
    }
    return QStringLiteral("未知");
}
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , root(nullptr)
{
    ui->setupUi(this);

    queryManager = new QueryManager(this);
    setupHistoryDock();
    connect(queryManager, &QueryManager::historyChanged, this, &MainWindow::refreshHistoryTable);

    statusBar()->showMessage(tr("应用已启动，等待构建四叉树"), 5000);

    // 设置 label 的事件过滤器，仅响应点击

    ui->label->installEventFilter(this);

    // 创建 drawWidget 作为 label 的子控件
    drawWidget = new DrawWidget(ui->label);
    drawWidget->setGeometry(ui->label->rect());
    drawWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    drawWidget->setAttribute(Qt::WA_TranslucentBackground);
    drawWidget->setStyleSheet("background: transparent;");
    drawWidget->show();

    // 创建 overlay 作为 label 的子控件
    overlay = new OverlayWidget(ui->label);
    overlay->setGeometry(ui->label->rect());
    overlay->setAttribute(Qt::WA_TranslucentBackground);
    overlay->setStyleSheet("background: transparent;");
    overlay->show();


    // 连接矩形完成信号
    connect(overlay, &OverlayWidget::rectangleReady, this, &MainWindow::updateLineEdits);
}



MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);


}

void MainWindow::on_label_2_linkActivated(const QString &link)
{

}
void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if (overlay && ui->label) {
        overlay->setGeometry(ui->label->geometry());
    }
}


void MainWindow::updateLineEdits(const QPoint &p1, const QPoint &p2) {
    QPoint tl(std::min(p1.x(), p2.x()), std::min(p1.y(), p2.y()));
    QPoint br(std::max(p1.x(), p2.x()), std::max(p1.y(), p2.y()));

    // 线性变换参数
    constexpr double scale1 = 0.0013043478261;
    constexpr double offset1 = 116.0047826086956;

    constexpr double scale2 = -0.00092378752886;
    constexpr double offset2 = 40.16189376443418;

    // 应用变换
    double transformedX1 = tl.x() * scale1 + offset1;
    double transformedX2 = br.x() * scale1 + offset1;
    double transformedY1 = tl.y() * scale2 + offset2;
    double transformedY2 = br.y() * scale2 + offset2;

    ui->lineEdit_3->setText(QString::number(transformedX1, 'f', 6));  // 保留6位小数
    ui->lineEdit_4->setText(QString::number(transformedY1, 'f', 6));
    ui->lineEdit_5->setText(QString::number(transformedX2, 'f', 6));
    ui->lineEdit_6->setText(QString::number(transformedY2, 'f', 6));
}

void MainWindow::setupHistoryDock() {
    if (!ui->dockWidgetHistory) {
        return;
    }

    ui->dockWidgetHistory->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    ui->dockWidgetHistory->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    if (!ui->tableWidgetHistory) {
        return;
    }

    ui->tableWidgetHistory->setColumnCount(4);
    ui->tableWidgetHistory->setHorizontalHeaderLabels(QStringList{
        tr("类型"), tr("参数"), tr("结果数量"), tr("时间")});
    ui->tableWidgetHistory->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidgetHistory->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidgetHistory->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidgetHistory->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidgetHistory->verticalHeader()->setVisible(false);
}

void MainWindow::refreshHistoryTable(const QVector<QueryRecord> &records) {
    if (!ui->tableWidgetHistory) {
        return;
    }

    ui->tableWidgetHistory->setRowCount(records.size());

    for (int row = 0; row < records.size(); ++row) {
        const auto &record = records.at(row);

        auto *typeItem = new QTableWidgetItem(queryTypeToString(record.type));
        typeItem->setData(Qt::UserRole, row);
        auto *paramItem = new QTableWidgetItem(toDisplayString(record));
        auto *countItem = new QTableWidgetItem(QString::number(record.resultCount));
        auto *timeItem = new QTableWidgetItem(record.timestamp.toString("yyyy-MM-dd HH:mm:ss"));

        ui->tableWidgetHistory->setItem(row, 0, typeItem);
        ui->tableWidgetHistory->setItem(row, 1, paramItem);
        ui->tableWidgetHistory->setItem(row, 2, countItem);
        ui->tableWidgetHistory->setItem(row, 3, timeItem);
    }

    ui->tableWidgetHistory->resizeColumnsToContents();
}

QString MainWindow::toDisplayString(const QueryRecord &record) const {
    if (record.parameters.isEmpty()) {
        return tr("无");
    }

    QStringList parts;
    const auto keys = record.parameters.keys();
    for (const auto &key : keys) {
        parts << QStringLiteral("%1=%2").arg(key, record.parameters.value(key).toString());
    }
    return parts.join(QStringLiteral(", "));
}

void MainWindow::registerQueryResult(QueryType type, const QVariantMap &parameters, int resultCount) {
    if (!queryManager) {
        return;
    }

    QueryRecord record;
    record.type = type;
    record.parameters = parameters;
    record.resultCount = resultCount;
    record.timestamp = QDateTime::currentDateTime();
    queryManager->addRecord(record);

    const QString message = tr("%1完成，匹配%2条记录")
                                .arg(queryTypeToString(type))
                                .arg(resultCount);
    statusBar()->showMessage(message, 5000);
}


void MainWindow::rerunRecord(const QueryRecord &record) {
    if (!root) {
        QMessageBox::information(this, tr("提示"), tr("请先构建四叉树"));
        return;
    }

    const auto params = record.parameters;
    switch (record.type) {
    case QueryType::Point: {
        ui->lineEdit->setText(params.value(QStringLiteral("经度")).toString());
        ui->lineEdit_2->setText(params.value(QStringLiteral("纬度")).toString());
        on_pushButton_clicked();
        break;
    }
    case QueryType::Area: {
        ui->lineEdit_3->setText(params.value(QStringLiteral("最小经度")).toString());
        ui->lineEdit_4->setText(params.value(QStringLiteral("最小纬度")).toString());
        ui->lineEdit_5->setText(params.value(QStringLiteral("最大经度")).toString());
        ui->lineEdit_6->setText(params.value(QStringLiteral("最大纬度")).toString());
        on_pushButton_3_clicked();
        break;
    }
    case QueryType::Adjacency: {
        ui->lineEdit_3->setText(params.value(QStringLiteral("最小经度")).toString());
        ui->lineEdit_4->setText(params.value(QStringLiteral("最小纬度")).toString());
        ui->lineEdit_5->setText(params.value(QStringLiteral("最大经度")).toString());
        ui->lineEdit_6->setText(params.value(QStringLiteral("最大纬度")).toString());
        ui->lineEdit_7->setText(params.value(QStringLiteral("开始时间")).toString());
        ui->lineEdit_8->setText(params.value(QStringLiteral("结束时间")).toString());
        ui->lineEdit_9->setText(params.value(QStringLiteral("车辆ID")).toString());
        on_pushButton_4_clicked();
        break;
    }
    case QueryType::Trajectory: {
        ui->lineEdit_3->setText(params.value(QStringLiteral("最小经度")).toString());
        ui->lineEdit_4->setText(params.value(QStringLiteral("最小纬度")).toString());
        ui->lineEdit_5->setText(params.value(QStringLiteral("最大经度")).toString());
        ui->lineEdit_6->setText(params.value(QStringLiteral("最大纬度")).toString());
        ui->lineEdit_9->setText(params.value(QStringLiteral("车辆ID")).toString());
        on_pushButton_5_clicked();
        break;
    }
    }
}


bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui->label && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QPoint localPos = mouseEvent->pos();  // 鼠标相对于 label 的位置

        // 将事件坐标传递给 overlay
        if (overlay) {
            overlay->setPoint(localPos);
            overlay->update();  // 触发重绘
        }

        return true; // 表示事件已处理
    }
    return QMainWindow::eventFilter(watched, event);
}



void MainWindow::on_pushButton_clicked()
{
    if (!root) {
        ui->textEdit->setText(tr("请先构建四叉树"));
        statusBar()->showMessage(tr("请先构建四叉树后再执行查询"), 5000);
        return;
    }

    QString lon = ui->lineEdit->text();
    double x = lon.toDouble();
    QString lan = ui->lineEdit_2->text();
    double y = lan.toDouble();

    qDebug() << "输入经度：" << lon << " 转换后：" << x;
    qDebug() << "输入纬度：" << lan << " 转换后：" << y;
    auto result = root->PointSearch(std::make_pair(x, y));
    QVariantMap params;
    params.insert(QStringLiteral("经度"), lon);
    params.insert(QStringLiteral("纬度"), lan);
    registerQueryResult(QueryType::Point, params, static_cast<int>(result.size()));

    if (result.empty()) {
        ui->textEdit->setText(tr("未查询到匹配轨迹"));
        return;
    }

    printVector(result);

    QFile file("output.txt");

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ui->textEdit->setText(tr("无法打开文件！"));
        return;
    }

    QTextStream in(&file);
    const QString fileContent = in.readAll();
    file.close();

    ui->textEdit->setText(fileContent);
}


void MainWindow::on_pushButton_2_clicked()
{
    if (queryManager) {
        queryManager->clear();
    }

    statusBar()->showMessage(tr("开始构建四叉树..."));

    this->root = new QuadNode();   // 正确地使用成员变量
    int DATANUM = 10000;
    Rectangle* bounding_box = new Rectangle();
    double min_lon = 181, max_lon = 0, min_lat = 181, max_lat = 0;

    ui->label_2->setText("正在构建包围盒");

    for (int i = 1; i <= DATANUM; ++i) {
        std::string buffer;
        std::string path = "E:\\Code\\GIS\\Sichashu\\Quadtree\\data\\" + std::to_string(i) + ".txt";
        std::ifstream file(path);
        if (!file.is_open()) continue;

        while (std::getline(file, buffer)) {
            auto gps = str2data(buffer);
            min_lon = std::min(min_lon, gps->longitude);
            min_lat = std::min(min_lat, gps->latitude);
            max_lon = std::max(max_lon, gps->longitude);
            max_lat = std::max(max_lat, gps->latitude);
            delete gps;
        }

        file.close();
    }

    bounding_box->bottom_left = std::make_pair(min_lon, min_lat);
    bounding_box->top_right = std::make_pair(max_lon, max_lat);
    this->root->range = bounding_box;

    ui->label_2->setText("正在构建四叉树");

    for (int i = 1; i <= DATANUM; ++i) {
        std::string buffer;
        std::string path = "E:\\Code\\GIS\\Sichashu\\Quadtree\\data\\" + std::to_string(i) + ".txt";
        std::ifstream file(path);
        if (!file.is_open()) continue;

        while (std::getline(file, buffer)) {
            auto gps = str2data(buffer);
            this->root->InsertNode(gps, 8);  // 用 this->root 插入数据
        }

        file.close();
    }

    ui->label_2->setText("当前进度：四叉树已构建完成");
    statusBar()->showMessage(tr("四叉树构建完成"), 5000);
}



void MainWindow::on_pushButton_3_clicked()
{
    if (!root) {
        ui->textEdit->setText(tr("请先构建四叉树"));
        statusBar()->showMessage(tr("请先构建四叉树后再执行查询"), 5000);
        return;
    }

    QString lon1 = ui->lineEdit_3->text();
    double x1 = lon1.toDouble();
    QString lan1 = ui->lineEdit_4->text();
    double y1 = lan1.toDouble();
    QString lon2 = ui->lineEdit_5->text();
    double x2 = lon2.toDouble();
    QString lan2 = ui->lineEdit_6->text();
    double y2 = lan2.toDouble();

    Rectangle* rect = new Rectangle(x2, y1, x1, y2);
    //Rectangle* rect = new Rectangle(y1, x2, y2, x1);
    auto result = root->AreaSearch(rect);
    QVariantMap params;
    params.insert(QStringLiteral("最小经度"), QString::number(std::min(x1, x2)));
    params.insert(QStringLiteral("最大经度"), QString::number(std::max(x1, x2)));
    params.insert(QStringLiteral("最小纬度"), QString::number(std::min(y1, y2)));
    params.insert(QStringLiteral("最大纬度"), QString::number(std::max(y1, y2)));
    registerQueryResult(QueryType::Area, params, static_cast<int>(result.size()));

    if (result.empty()) {
        ui->textEdit->setText(tr("未查询到匹配轨迹"));
        delete rect;
        return;
    }

    printVector(result);

    QFile file("output.txt");

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ui->textEdit->setText(tr("无法打开文件！"));
        delete rect;
        return;
    }

    QTextStream in(&file);
    const QString fileContent = in.readAll();
    file.close();

    ui->textEdit->setText(fileContent);
    delete rect;
}


void MainWindow::on_pushButton_4_clicked()
{
    if (!root) {
        ui->textEdit->setText(tr("请先构建四叉树"));
        statusBar()->showMessage(tr("请先构建四叉树后再执行查询"), 5000);
        return;
    }

    QString lon1 = ui->lineEdit_3->text();
    double x1 = lon1.toDouble();
    QString lan1 = ui->lineEdit_4->text();
    double y1 = lan1.toDouble();
    QString lon2 = ui->lineEdit_5->text();
    double x2 = lon2.toDouble();
    QString lan2 = ui->lineEdit_6->text();
    double y2 = lan2.toDouble();

    Rectangle* rect = new Rectangle(x2, y1, x1, y2);

    std::vector<int> time1, time2;

    QString timeStr1 = ui->lineEdit_7->text();  // 例如 "2025-05-05 14:30:15"
    QStringList parts1 = timeStr1.split(" ");
    if (parts1.size() == 2) {
        QStringList date1 = parts1[0].split("-");
        QStringList clock1 = parts1[1].split(":");
        if (date1.size() == 3 && clock1.size() == 3) {
            time1.push_back(date1[0].toInt()); // 年
            time1.push_back(date1[1].toInt()); // 月
            time1.push_back(date1[2].toInt()); // 日
            time1.push_back(clock1[0].toInt()); // 时
            time1.push_back(clock1[1].toInt()); // 分
            time1.push_back(clock1[2].toInt()); // 秒
        }
    }

    QString timeStr2 = ui->lineEdit_8->text();  // 例如 "2025-05-05 15:01:30"
    QStringList parts2 = timeStr2.split(" ");
    if (parts2.size() == 2) {
        QStringList date2 = parts2[0].split("-");
        QStringList clock2 = parts2[1].split(":");
        if (date2.size() == 3 && clock2.size() == 3) {
            time2.push_back(date2[0].toInt()); // 年
            time2.push_back(date2[1].toInt()); // 月
            time2.push_back(date2[2].toInt()); // 日
            time2.push_back(clock2[0].toInt()); // 时
            time2.push_back(clock2[1].toInt()); // 分
            time2.push_back(clock2[2].toInt()); // 秒
        }
    }
    QString idxString = ui->lineEdit_9->text();
    double idx = idxString.toDouble();

    auto result = root->AdjacentSearch(rect, time1, time2, idx);
    QVariantMap params;
    params.insert(QStringLiteral("最小经度"), QString::number(std::min(x1, x2)));
    params.insert(QStringLiteral("最大经度"), QString::number(std::max(x1, x2)));
    params.insert(QStringLiteral("最小纬度"), QString::number(std::min(y1, y2)));
    params.insert(QStringLiteral("最大纬度"), QString::number(std::max(y1, y2)));
    params.insert(QStringLiteral("开始时间"), ui->lineEdit_7->text());
    params.insert(QStringLiteral("结束时间"), ui->lineEdit_8->text());
    params.insert(QStringLiteral("车辆ID"), ui->lineEdit_9->text());
    registerQueryResult(QueryType::Adjacency, params, result ? 1 : 0);

    if (result == nullptr) {
        ui->textEdit->setText(tr("未查询到匹配轨迹"));
        delete rect;
        return;
    }

    QFile::remove("output.txt");
    result->print();

    QFile file("output.txt");

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ui->textEdit->setText(tr("无法打开文件！"));
        delete rect;
        return;
    }

    QTextStream in(&file);
    const QString fileContent = in.readAll();
    file.close();

    ui->textEdit->setText(fileContent);
    delete rect;
}



void MainWindow::on_pushButton_5_clicked()
{
    if (!root) {
        ui->textEdit->setText(tr("请先构建四叉树"));
        statusBar()->showMessage(tr("请先构建四叉树后再执行查询"), 5000);
        return;
    }

    QString lon1 = ui->lineEdit_3->text();
    double x1 = lon1.toDouble();
    QString lan1 = ui->lineEdit_4->text();
    double y1 = lan1.toDouble();
    QString lon2 = ui->lineEdit_5->text();
    double x2 = lon2.toDouble();
    QString lan2 = ui->lineEdit_6->text();
    double y2 = lan2.toDouble();
    QString idxString = ui->lineEdit_9->text();
    int idx = idxString.toInt();
    Rectangle* rect = new Rectangle(x2, y1, x1, y2);
    //Rectangle* rect = new Rectangle(y1, x2, y2, x1);
    auto result = root->TrajectorySearch(rect,idx);
    QVariantMap params;
    params.insert(QStringLiteral("最小经度"), QString::number(std::min(x1, x2)));
    params.insert(QStringLiteral("最大经度"), QString::number(std::max(x1, x2)));
    params.insert(QStringLiteral("最小纬度"), QString::number(std::min(y1, y2)));
    params.insert(QStringLiteral("最大纬度"), QString::number(std::max(y1, y2)));
    params.insert(QStringLiteral("车辆ID"), ui->lineEdit_9->text());
    registerQueryResult(QueryType::Trajectory, params, static_cast<int>(result.size()));

    if (result.empty()) {
        ui->textEdit->setText(tr("未查询到匹配轨迹"));
        delete rect;
        return;
    }

    printVector(result);

    QFile file("output.txt");

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ui->textEdit->setText(tr("无法打开文件！"));
        delete rect;
        return;
    }

    QTextStream in(&file);
    const QString fileContent = in.readAll();
    file.close();

    ui->textEdit->setText(fileContent);

    drawWidget->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    drawWidget->loadPointsFromFile("output.txt");
    drawWidget->update();

    delete rect;
}


void MainWindow::on_pushButton_6_clicked()
{
    if (!root) {
        ui->textEdit->setText(tr("请先构建四叉树"));
        statusBar()->showMessage(tr("请先构建四叉树后再执行操作"), 5000);
        return;
    }

    QString lon = ui->lineEdit->text();
    double x = lon.toDouble();
    QString lan = ui->lineEdit_2->text();
    double y = lan.toDouble();
    QString newlon = ui->lineEdit_11->text();
    double newx = newlon.toDouble();
    QString newlan = ui->lineEdit_10->text();
    double newy = newlan.toDouble();
    QString idx = ui->lineEdit_9->text();
    int id = idx.toDouble();
    qDebug() << "输入经度：" << lon << " 转换后：" << x;
    qDebug() << "输入纬度：" << lan << " 转换后：" << y;
    auto result = root->PointChange(id,std::make_pair(x, y),newx,newy);
    if (!result) {
        ui->textEdit->setText(tr("未找到对应轨迹，修改失败"));
        statusBar()->showMessage(tr("轨迹修改失败"), 4000);
    } else {
        ui->textEdit->setText(tr("轨迹修改成功"));
        statusBar()->showMessage(tr("轨迹修改成功"), 4000);
    }
}


void MainWindow::on_pushButton_7_clicked()
{
    if (!root) {
        ui->textEdit->setText(tr("请先构建四叉树"));
        statusBar()->showMessage(tr("请先构建四叉树后再执行操作"), 5000);
        return;
    }

    QString lon = ui->lineEdit->text();
    double x = lon.toDouble();
    QString lan = ui->lineEdit_2->text();
    double y = lan.toDouble();
    QString idx = ui->lineEdit_9->text();
    int id = idx.toDouble();
    qDebug() << "输入经度：" << lon << " 转换后：" << x;
    qDebug() << "输入纬度：" << lan << " 转换后：" << y;
    auto result = root->PointDelete(id,x, y);
    if (!result) {
        ui->textEdit->setText(tr("未找到对应轨迹，删除失败"));
        statusBar()->showMessage(tr("轨迹删除失败"), 4000);
    } else {
        ui->textEdit->setText(tr("轨迹删除成功"));
        statusBar()->showMessage(tr("轨迹删除成功"), 4000);
    }
}

void MainWindow::on_pushButtonRerunQuery_clicked()
{
    if (!queryManager || !ui->tableWidgetHistory) {
        return;
    }

    const auto *selection = ui->tableWidgetHistory->selectionModel();
    if (!selection || selection->selectedRows().isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请选择需要重新执行的历史记录"));
        return;
    }

    const int row = selection->selectedRows().first().row();
    if (row < 0 || row >= queryManager->recordCount()) {
        QMessageBox::warning(this, tr("警告"), tr("所选记录无效"));
        return;
    }
    const QueryRecord record = queryManager->record(row);
    rerunRecord(record);
}

void MainWindow::on_pushButtonExportResult_clicked()
{
    const QString content = ui->textEdit->toPlainText();
    if (content.trimmed().isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("没有可导出的查询结果"));
        return;
    }

    const QString defaultName = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_查询结果.txt");
    const QString filePath = QFileDialog::getSaveFileName(this,
                                                          tr("导出查询结果"),
                                                          defaultName,
                                                          tr("文本文件 (*.txt);;所有文件 (*.*)"));
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("错误"), tr("无法写入文件"));
        return;
    }

    QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    out.setEncoding(QStringConverter::Utf8);
#else
    out.setCodec("UTF-8");
#endif
    out << content;
    file.close();

    statusBar()->showMessage(tr("查询结果已导出至%1").arg(filePath), 5000);
}

void MainWindow::on_pushButtonClearHistory_clicked()
{
    if (!queryManager) {
        return;
    }

    queryManager->clear();
    statusBar()->showMessage(tr("查询历史已清空"), 3000);
}

