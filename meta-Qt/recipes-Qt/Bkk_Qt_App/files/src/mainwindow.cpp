#include "mainwindow.hpp"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QPixmap>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent),
      clockLabel(nullptr),
      bkkLogoLabel(nullptr),
      wifiIconLabel(nullptr),
    arrivalsTable(nullptr),
    blinkOn(false)
{
    setupUi();
    startTimers();
}

MainWindow::~MainWindow()
{
    stopTimers();
}

void MainWindow::setupUi()
{
    setMinimumSize(480, 320);
    setWindowTitle("BKK Display");

    setStyleSheet(
        "QWidget { background-color: #340a41; color: #ffffff; }"
        "QLabel  { background-color: #340a41; color: #ffffff; }"
        "QHeaderView::section { background-color: #505050; color: #ffffff; "
        "                        border: none; padding: 4px; font-weight: bold; }"
        "QTableWidget { background-color: #340a41; gridline-color: #505050;"
        "                border: none; }"
        "QTableCornerButton::section { background-color: #505050; }"
    );

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    auto *statusRow = new QWidget(this);
    statusRow->setFixedHeight(46);
    auto *statusRowLayout = new QHBoxLayout(statusRow);
    statusRowLayout->setContentsMargins(0, 0, 0, 0);
    statusRowLayout->setSpacing(8);

    clockLabel = new QLabel(statusRow);
    clockLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    clockLabel->setFixedWidth(86);

    bkkLogoLabel = new QLabel(statusRow);
    bkkLogoLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    bkkLogoLabel->setFixedSize(106, 40);
    statusRowLayout->addWidget(bkkLogoLabel);

    statusRowLayout->addStretch(1);

    wifiIconLabel = new QLabel(statusRow);
    wifiIconLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    wifiIconLabel->setFixedSize(40, 40);
    statusRowLayout->addWidget(wifiIconLabel);

    statusRowLayout->addSpacing(12);

    statusRowLayout->addWidget(clockLabel);

    layout->addWidget(statusRow);

    arrivalsTable = new QTableWidget(this);
    arrivalsTable->setColumnCount(4);
    arrivalsTable->setHorizontalHeaderLabels(
        {"Station", "Line", "Destination", "Departs"});
    setupTableWidget();
    layout->addWidget(arrivalsTable, 1);
}

void MainWindow::setupTableWidget()
{
    arrivalsTable->horizontalHeader()->setStretchLastSection(false);
    arrivalsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    arrivalsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    arrivalsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    arrivalsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    arrivalsTable->verticalHeader()->setVisible(false);
    arrivalsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    arrivalsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    arrivalsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    arrivalsTable->setAlternatingRowColors(false);
    arrivalsTable->setSortingEnabled(false);
}

void MainWindow::startTimers()
{
    clockUpdater.update();
    apiWrapper.fetchData();
    onlineChecker.cyclicCheck();

    QObject::connect(&clockUpdateTimer, &QTimer::timeout, this, [this]() {
        clockUpdater.update();
    });
    clockUpdateTimer.start(1000);

    QObject::connect(&bkkApiFetchTimer, &QTimer::timeout, this, [this]() {
        apiWrapper.fetchData();
    });
    bkkApiFetchTimer.start(10000);

    QObject::connect(&onlineCheckTimer, &QTimer::timeout, this, [this]() {
        onlineChecker.cyclicCheck();
    });
    onlineCheckTimer.start(5000);

    QObject::connect(&mainTaskTimer, &QTimer::timeout, this, [this]() {
        updateUi();
    });
    mainTaskTimer.start(1000);

    updateUi();
}

void MainWindow::stopTimers()
{
    mainTaskTimer.stop();
    onlineCheckTimer.stop();
    bkkApiFetchTimer.stop();
    clockUpdateTimer.stop();
}

void MainWindow::updateUi()
{
    const bool connected = onlineChecker.isOnline();
    blinkOn = !blinkOn;

    clockLabel->setText(QString::fromStdString(clockUpdater.getClock()));

    wifiIconLabel->setPixmap(
        QPixmap(connected ? ":/icons/wifi_on.png" : ":/icons/wifi_off.png")
            .scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    bkkLogoLabel->setPixmap(
        QPixmap(":/icons/bkk_logo.png")
            .scaled(106, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    populateArrivalsTable();
}

void MainWindow::populateArrivalsTable()
{
    auto arrivals = apiWrapper.getArrivals();
    static constexpr int kMaxRows = 8;

    std::sort(arrivals.begin(), arrivals.end(), [](const StationArrival &left, const StationArrival &right) {
        return left.arrival.departs_in_min < right.arrival.departs_in_min;
    });

    if (static_cast<int>(arrivals.size()) > kMaxRows) {
        arrivals.resize(static_cast<size_t>(kMaxRows));
    }

    if (apiWrapper.getErrorCode() != BkkApiError::None) {
        showTableMessage("Error fetching arrivals");
        return;
    }

    if (arrivals.empty()) {
        showTableMessage("No arrivals");
        return;
    }

    arrivalsTable->clearContents();
    arrivalsTable->clearSpans();
    arrivalsTable->setRowCount(static_cast<int>(arrivals.size()));

    for (int row = 0; row < static_cast<int>(arrivals.size()); ++row) {
        const auto &stationArrival = arrivals[static_cast<size_t>(row)];
        const QColor backgroundColor = getRowColor(row);

        auto *stopItem = new QTableWidgetItem(QString::fromStdString(stationArrival.station_name));
        stopItem->setTextAlignment(Qt::AlignCenter);
        stopItem->setBackground(backgroundColor);
        stopItem->setForeground(Qt::white);
        arrivalsTable->setItem(row, 0, stopItem);

        auto *lineItem = new QTableWidgetItem(QString::fromStdString(stationArrival.arrival.line));
        lineItem->setTextAlignment(Qt::AlignCenter);
        lineItem->setBackground(backgroundColor);
        lineItem->setForeground(Qt::white);
        arrivalsTable->setItem(row, 1, lineItem);

        auto *destinationItem = new QTableWidgetItem(QString::fromStdString(stationArrival.arrival.destination));
        destinationItem->setBackground(backgroundColor);
        destinationItem->setForeground(Qt::white);
        arrivalsTable->setItem(row, 2, destinationItem);

        arrivalsTable->setCellWidget(row, 3,
            createDepartureCell(stationArrival.arrival.departs_in_min, backgroundColor));

    }

    arrivalsTable->resizeRowsToContents();
}

void MainWindow::showTableMessage(const QString &message)
{
    arrivalsTable->clearContents();
    arrivalsTable->clearSpans();
    arrivalsTable->setRowCount(1);

    auto *messageItem = new QTableWidgetItem(message);
    messageItem->setTextAlignment(Qt::AlignCenter);
    arrivalsTable->setSpan(0, 0, 1, arrivalsTable->columnCount());
    arrivalsTable->setItem(0, 0, messageItem);
}

QColor MainWindow::getRowColor(int row) const
{
    return (row % 2 == 0) ? QColor("#340a41") : QColor("#505050");
}

QWidget *MainWindow::createDepartureCell(int departsInMin, const QColor &backgroundColor) const
{
    auto *container = new QWidget(arrivalsTable);
    container->setStyleSheet(QString("background-color: %1;").arg(backgroundColor.name()));

    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(6, 0, 6, 0);
    layout->setSpacing(6);
    layout->setAlignment(Qt::AlignCenter);

    auto *dot = new QLabel(container);
    dot->setFixedSize(8, 8);

    QString dotColor = "transparent";
    if (blinkOn) {
        if (departsInMin < 5) {
            dotColor = "#ff2d2d";
        } else if (departsInMin <= 10) {
            dotColor = "#00d84f";
        }
    }
    dot->setStyleSheet(QString("background-color: %1; border-radius: 4px;").arg(dotColor));

    auto *minutes = new QLabel(QString::number(departsInMin) + "'", container);
    minutes->setAlignment(Qt::AlignCenter);
    minutes->setStyleSheet("color: #ffffff;");

    layout->addWidget(dot);
    layout->addWidget(minutes);
    return container;
}