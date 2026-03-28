#include "mainwindow.hpp"
#include "bkk_touchscreen.hpp"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QPixmap>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <algorithm>
#include <cstdio>


MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent),
      clockLabel(nullptr),
      bkkLogoLabel(nullptr),
      wifiIconLabel(nullptr),
      arrivalsTable(nullptr),
      apiError(BkkApiError::None),
            clockText(),
            onlineStatus(false),
      blinkOn(false)
{
    setupUi();
    startWorkerThread();
    startTimers();

    // Initialize touchscreen callback
    setupTouchScreenWorker(); 
}

MainWindow::~MainWindow()
{
    stopTimers();
    stopWorkerThread();
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

void MainWindow::startWorkerThread()
{
    QObject::connect(&workerThread, &WorkerThread::apiFetchCompleted, this, &MainWindow::handleApiFetchCompleted);
    QObject::connect(&workerThread, &WorkerThread::clockUpdateCompleted, this, &MainWindow::handleClockUpdateCompleted);
    QObject::connect(&workerThread, &WorkerThread::onlineCheckCompleted, this, &MainWindow::handleOnlineCheckCompleted);

    if (!workerThread.isRunning()) {
        workerThread.start();
    }
}

void MainWindow::stopWorkerThread()
{
    if (!workerThread.isRunning()) {
        return;
    }

    workerThread.requestInterruption();
    workerThread.wait();
}

void MainWindow::startTimers()
{
    workerThread.requestClockUpdate();
    workerThread.requestOnlineCheck();
    workerThread.requestApiFetch();

    QObject::connect(&clockUpdateTimer, &QTimer::timeout, this, [this]() {
        workerThread.requestClockUpdate();
    });
    clockUpdateTimer.start(1000);

    QObject::connect(&bkkApiFetchTimer, &QTimer::timeout, this, [this]() {
        workerThread.requestApiFetch();
    });
    bkkApiFetchTimer.start(10000);

    QObject::connect(&onlineCheckTimer, &QTimer::timeout, this, [this]() {
        workerThread.requestOnlineCheck();
    });
    onlineCheckTimer.start(5000);

    QObject::connect(&mainTaskTimer, &QTimer::timeout, this, [this]() {
        updateUi();

        // flip the blink state for the departure dots
        blinkOn = !blinkOn;
    });
    mainTaskTimer.start(1000);

    updateUi();
}

void MainWindow::handleApiFetchCompleted()
{
    arrivals = workerThread.getArrivals();
    apiError = workerThread.getErrorCode();
    updateUi();
}

void MainWindow::handleClockUpdateCompleted()
{
    clockText = workerThread.getClockText();
    updateUi();
}

void MainWindow::handleOnlineCheckCompleted()
{
    onlineStatus = workerThread.isOnline();
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
    clockLabel->setText(QString::fromStdString(clockText));

    wifiIconLabel->setPixmap(
        QPixmap(onlineStatus ? ":/icons/wifi_on.png" : ":/icons/wifi_off.png")
            .scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    bkkLogoLabel->setPixmap(
        QPixmap(":/icons/bkk_logo.png")
            .scaled(106, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    populateArrivalsTable();
}

void MainWindow::populateArrivalsTable()
{
    auto currentArrivals = arrivals;
    static constexpr int kMaxRows = 8;

    std::sort(currentArrivals.begin(), currentArrivals.end(), [](const StationArrival &left, const StationArrival &right) {
        return left.arrival.departs_in_min < right.arrival.departs_in_min;
    });

    if (static_cast<int>(currentArrivals.size()) > kMaxRows) {
        currentArrivals.resize(static_cast<size_t>(kMaxRows));
    }

    if (apiError != BkkApiError::None) {
        showTableMessage("Error fetching arrivals");
        return;
    }

    if (currentArrivals.empty()) {
        showTableMessage("No arrivals");
        return;
    }

    arrivalsTable->clearContents();
    arrivalsTable->clearSpans();
    arrivalsTable->setRowCount(static_cast<int>(currentArrivals.size()));

    for (int row = 0; row < static_cast<int>(currentArrivals.size()); ++row) {
        const auto &stationArrival = currentArrivals[static_cast<size_t>(row)];
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

void MainWindow::setupTouchScreenWorker() {
    touchscreenWorker = new BkkTouchScreenWorker(
        touchscreenCallback, this);

    // setup worker timer but do not start it yet:
    QObject::connect(
        &touchScreenWorkerTimer, 
        &QTimer::timeout, this, [this]() {
            if (touchscreenWorker != nullptr) {
                touchscreenWorker->fetch_touch_coordinates();
            }
        }
    );
}


void MainWindow::touchscreenCallback(ts_event_en event, void * arg) {
    (void)event; // Unused parameter
    
    MainWindow *self = static_cast<MainWindow *>(arg);
    if (self == nullptr) {
        return;
    }

    if(event == TOUCHSCREEN_EVENT_TOUCHED) {
        self->currentTouchEvent = TOUCHSCREEN_EVENT_TOUCHED;
        // Start timer to fetch coordinates 
        self->touchScreenWorkerTimer.start(30); 
    }
    else if(event == TOUCHSCREEN_EVENT_RELEASED) {
        self->currentTouchEvent = TOUCHSCREEN_EVENT_RELEASED;
        // Stop fetching coordinates when touch is released
        self->touchScreenWorkerTimer.stop();
    }
    else {
        self->currentTouchEvent = TOUCHSCREEN_EVENT_RELEASED;
        self->touchScreenWorkerTimer.stop();
    }    
}