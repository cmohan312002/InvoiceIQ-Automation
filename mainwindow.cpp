#include "mainwindow.h"
#include "invoiceviewdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QTextEdit>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QGroupBox>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QPixmap>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMediaDevices>
#include <QCameraDevice>
#include <QVideoWidget>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QImageCapture>
#include <QComboBox>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("InvoiceData — Aether Solutions");
    setMinimumSize(1200, 750);
    applyStyleSheet();
    buildMenuBar();
    buildToolBar();
    buildUI();

    // Init OCR
    if (!m_ocr.initialize()) {
        statusBar()->showMessage("⚠ Tesseract not ready: " + m_ocr.lastError());
    } else {
        statusBar()->showMessage("✔ OCR Engine ready");
    }

    // Init DB
    if (!m_db.open("invoices.db")) {
        QMessageBox::critical(this, "DB Error", m_db.lastError());
    } else {
        refreshTable();
    }
}

MainWindow::~MainWindow() {
    if (m_camera) {
        m_camera->stop();
        delete m_camera;
    }
}

// ── UI ───────────────────────────────────────────────────────────────────────
void MainWindow::buildUI() {
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(8,8,8,8);

    // Search bar
    QHBoxLayout *searchRow = new QHBoxLayout;
    m_searchBox = new QLineEdit;
    m_searchBox->setPlaceholderText("🔍  Search by invoice #, from, or to...");
    m_searchBox->setFixedHeight(34);
    QPushButton *searchBtn = new QPushButton("Search");
    QPushButton *clearBtn  = new QPushButton("Clear");
    searchBtn->setFixedHeight(34);
    clearBtn->setFixedHeight(34);
    searchRow->addWidget(m_searchBox);
    searchRow->addWidget(searchBtn);
    searchRow->addWidget(clearBtn);
    connect(searchBtn,   &QPushButton::clicked,      this, &MainWindow::onSearch);
    connect(clearBtn,    &QPushButton::clicked,      this, &MainWindow::onClearSearch);
    connect(m_searchBox, &QLineEdit::returnPressed,  this, &MainWindow::onSearch);

    // Splitter
    QSplitter *splitter = new QSplitter(Qt::Horizontal);

    // ── Left Panel ───────────────────────────────────────────────────
    QWidget *leftPanel = new QWidget;
    QVBoxLayout *lv = new QVBoxLayout(leftPanel);
    lv->setContentsMargins(0,0,0,0);

    m_table = new QTableWidget(0, 6);
    m_table->setHorizontalHeaderLabels({"DB ID","Invoice #","Date","From","To","Total ($)"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(true);
    connect(m_table, &QTableWidget::cellDoubleClicked,
            this, &MainWindow::onTableDoubleClick);

    QHBoxLayout *actRow = new QHBoxLayout;
    m_viewBtn   = new QPushButton("👁  View");
    m_editBtn   = new QPushButton("✏  Edit");
    m_deleteBtn = new QPushButton("🗑  Delete");
    m_viewBtn->setFixedHeight(32);
    m_editBtn->setFixedHeight(32);
    m_deleteBtn->setFixedHeight(32);
    actRow->addWidget(m_viewBtn);
    actRow->addWidget(m_editBtn);
    actRow->addWidget(m_deleteBtn);
    actRow->addStretch();

    connect(m_viewBtn,   &QPushButton::clicked, this, &MainWindow::onViewInvoice);
    connect(m_editBtn,   &QPushButton::clicked, this, &MainWindow::onEditInvoice);
    connect(m_deleteBtn, &QPushButton::clicked, this, &MainWindow::onDeleteInvoice);

    lv->addWidget(m_table);
    lv->addLayout(actRow);

    // ── Right Panel ──────────────────────────────────────────────────
    QWidget *rightPanel = new QWidget;
    QVBoxLayout *rv = new QVBoxLayout(rightPanel);
    rv->setContentsMargins(0,0,0,0);

    QGroupBox *prevGrp = new QGroupBox("Image Preview");
    QVBoxLayout *pvl = new QVBoxLayout(prevGrp);
    m_previewLabel = new QLabel("No image loaded");
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumHeight(220);
    m_previewLabel->setStyleSheet("background:#1e1e2e; border-radius:6px; color:#888;");
    pvl->addWidget(m_previewLabel);

    // Scan buttons inside right panel
    QHBoxLayout *scanRow = new QHBoxLayout;
    QPushButton *scanFileBtn   = new QPushButton("📂  Scan from File");
    QPushButton *scanCameraBtn = new QPushButton("📷  Scan from Camera");
    scanFileBtn->setFixedHeight(34);
    scanCameraBtn->setFixedHeight(34);
    scanRow->addWidget(scanFileBtn);
    scanRow->addWidget(scanCameraBtn);
    connect(scanFileBtn,   &QPushButton::clicked, this, &MainWindow::onScanImage);
    connect(scanCameraBtn, &QPushButton::clicked, this, &MainWindow::onScanCamera);

    QGroupBox *logGrp = new QGroupBox("Activity Log");
    QVBoxLayout *lgl = new QVBoxLayout(logGrp);
    m_logView = new QTextEdit;
    m_logView->setReadOnly(true);
    m_logView->setFont(QFont("Courier New", 9));
    m_logView->setMaximumHeight(200);
    lgl->addWidget(m_logView);

    m_progress = new QProgressBar;
    m_progress->setVisible(false);
    m_progress->setRange(0,0);

    rv->addWidget(prevGrp);
    rv->addLayout(scanRow);
    rv->addWidget(logGrp);
    rv->addWidget(m_progress);
    rv->addStretch();

    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    mainLayout->addLayout(searchRow);
    mainLayout->addWidget(splitter);

    m_viewBtn->setStyleSheet("background:#eb5f14; color:#fff; font-weight:600;");
    m_editBtn->setStyleSheet("background:#fff3ec; color:#eb5f14;");
    m_deleteBtn->setStyleSheet("background:#fff0f0; color:#d42d2d;");
}

// ── Menu Bar ─────────────────────────────────────────────────────────────────
void MainWindow::buildMenuBar() {
    QMenuBar *mb = menuBar();

    QMenu *fileMenu = mb->addMenu("&File");

    QAction *scanImgAct = new QAction("📂  Scan Image", this);
    scanImgAct->setShortcut(QKeySequence("Ctrl+O"));
    connect(scanImgAct, &QAction::triggered, this, &MainWindow::onScanImage);
    fileMenu->addAction(scanImgAct);

    QAction *scanCamAct = new QAction("📷  Scan Camera", this);
    scanCamAct->setShortcut(QKeySequence("Ctrl+K"));
    connect(scanCamAct, &QAction::triggered, this, &MainWindow::onScanCamera);
    fileMenu->addAction(scanCamAct);

    fileMenu->addSeparator();

    QAction *exportAct = new QAction("📤  Export CSV", this);
    exportAct->setShortcut(QKeySequence("Ctrl+E"));
    connect(exportAct, &QAction::triggered, this, &MainWindow::onExportCSV);
    fileMenu->addAction(exportAct);

    fileMenu->addSeparator();

    QAction *exitAct = new QAction("❌  Exit", this);
    exitAct->setShortcut(QKeySequence("Ctrl+Q"));
    connect(exitAct, &QAction::triggered, qApp, &QApplication::quit);
    fileMenu->addAction(exitAct);

    QMenu *invoiceMenu = mb->addMenu("&Invoice");

    QAction *viewAct = new QAction("👁  View", this);
    connect(viewAct, &QAction::triggered, this, &MainWindow::onViewInvoice);
    invoiceMenu->addAction(viewAct);

    QAction *editAct = new QAction("✏  Edit", this);
    connect(editAct, &QAction::triggered, this, &MainWindow::onEditInvoice);
    invoiceMenu->addAction(editAct);

    QAction *delAct = new QAction("🗑  Delete", this);
    connect(delAct, &QAction::triggered, this, &MainWindow::onDeleteInvoice);
    invoiceMenu->addAction(delAct);

    QAction *refreshAct = new QAction("🔄  Refresh", this);
    connect(refreshAct, &QAction::triggered, this, &MainWindow::refreshTable);
    invoiceMenu->addAction(refreshAct);

    QMenu *helpMenu = mb->addMenu("&Help");
    QAction *aboutAct = new QAction("About", this);
    connect(aboutAct, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "About",
                           "<b>InvoiceData</b><br>v1.0<br><br>"
                           "OCR-powered invoice digitizer<br>"
                           "Built with Qt6 + Tesseract + SQLite");
    });
    helpMenu->addAction(aboutAct);
}

// ── Tool Bar ─────────────────────────────────────────────────────────────────
void MainWindow::buildToolBar() {
    QToolBar *tb = addToolBar("Main");
    tb->setMovable(false);

    QAction *a1 = new QAction("📂 Scan Image", this);
    connect(a1, &QAction::triggered, this, &MainWindow::onScanImage);
    tb->addAction(a1);

    QAction *a2 = new QAction("📷 Scan Camera", this);
    connect(a2, &QAction::triggered, this, &MainWindow::onScanCamera);
    tb->addAction(a2);

    tb->addSeparator();

    QAction *a3 = new QAction("🔄 Refresh", this);
    connect(a3, &QAction::triggered, this, &MainWindow::refreshTable);
    tb->addAction(a3);

    QAction *a4 = new QAction("📤 Export CSV", this);
    connect(a4, &QAction::triggered, this, &MainWindow::onExportCSV);
    tb->addAction(a4);
}

// ── Camera Integration ───────────────────────────────────────────────────────
void MainWindow::onScanCamera() {

    // Get available cameras
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    if (cameras.isEmpty()) {
        QMessageBox::warning(this, "Camera",
                             "No camera found on this device.\n"
                             "Please connect a camera and try again.");
        return;
    }

    // ── Build camera selection dialog ────────────────────────────────
    m_cameraDialog = new QDialog(this);
    m_cameraDialog->setWindowTitle("📷 Camera Scanner");
    m_cameraDialog->setMinimumSize(700, 550);
    m_cameraDialog->setStyleSheet(styleSheet());

    QVBoxLayout *dlgLayout = new QVBoxLayout(m_cameraDialog);

    // Camera selector
    QHBoxLayout *camSelectRow = new QHBoxLayout;
    QLabel *camLabel = new QLabel("Select Camera:");
    QComboBox *camCombo = new QComboBox;
    camCombo->setFixedHeight(30);
    for (const QCameraDevice &dev : cameras)
        camCombo->addItem(dev.description());
    camSelectRow->addWidget(camLabel);
    camSelectRow->addWidget(camCombo);
    camSelectRow->addStretch();

    // Video preview widget
    m_videoWidget = new QVideoWidget;
    m_videoWidget->setMinimumHeight(380);
    m_videoWidget->setStyleSheet("background:#000;");

    // Status label
    QLabel *statusLbl = new QLabel("📹 Camera ready — position invoice and click Capture");
    statusLbl->setAlignment(Qt::AlignCenter);
    statusLbl->setStyleSheet("color:#a0c4ff; font-size:12px; padding:4px;");

    // Buttons
    QHBoxLayout *btnRow = new QHBoxLayout;
    QPushButton *captureBtn = new QPushButton("📸  Capture Invoice");
    QPushButton *cancelBtn  = new QPushButton("Cancel");
    captureBtn->setFixedHeight(38);
    cancelBtn->setFixedHeight(38);
    captureBtn->setStyleSheet(
        "QPushButton { background:#2ecc71; color:#000; font-weight:bold; "
        "border-radius:5px; padding:6px 20px; }"
        "QPushButton:hover { background:#27ae60; }"
        );
    btnRow->addStretch();
    btnRow->addWidget(captureBtn);
    btnRow->addWidget(cancelBtn);

    dlgLayout->addLayout(camSelectRow);
    dlgLayout->addWidget(m_videoWidget);
    dlgLayout->addWidget(statusLbl);
    dlgLayout->addLayout(btnRow);

    // ── Setup camera ─────────────────────────────────────────────────
    auto setupCamera = [&](int index) {
        if (m_camera) {
            m_camera->stop();
            delete m_camera;
            m_camera = nullptr;
        }
        if (m_captureSession) {
            delete m_captureSession;
            m_captureSession = nullptr;
        }
        if (m_imageCapture) {
            delete m_imageCapture;
            m_imageCapture = nullptr;
        }

        m_camera         = new QCamera(cameras[index], this);
        m_captureSession = new QMediaCaptureSession(this);
        m_imageCapture   = new QImageCapture(this);

        m_captureSession->setCamera(m_camera);
        m_captureSession->setVideoOutput(m_videoWidget);
        m_captureSession->setImageCapture(m_imageCapture);

        // Connect capture signals
        connect(m_imageCapture, &QImageCapture::imageCaptured,
                this, &MainWindow::onCameraCaptureDone);
        connect(m_imageCapture,
                &QImageCapture::errorOccurred,
                this, &MainWindow::onCameraError);

        m_camera->start();
        statusLbl->setText("📹 Camera active — position invoice and click Capture");
    };

    // Start with first camera
    setupCamera(0);

    // Switch camera on combo change
    connect(camCombo, &QComboBox::currentIndexChanged,
            this, [&](int idx){ setupCamera(idx); });

    // Capture button
    connect(captureBtn, &QPushButton::clicked, this, [&, statusLbl]() {
        if (!m_imageCapture) return;
        if (!m_imageCapture->isReadyForCapture()) {
            statusLbl->setText("⚠ Camera not ready yet, please wait...");
            return;
        }
        statusLbl->setText("📸 Capturing...");
        captureBtn->setEnabled(false);

        // Save to temp file and also get QImage via signal
        QString tempPath = QStandardPaths::writableLocation(
                               QStandardPaths::TempLocation)
                           + "/invoice_capture.jpg";
        m_imageCapture->captureToFile(tempPath);
        statusLbl->setText("✔ Captured! Processing OCR...");
    });

    // Cancel button
    connect(cancelBtn, &QPushButton::clicked, this, [&]() {
        if (m_camera) m_camera->stop();
        m_cameraDialog->reject();
    });

    // Stop camera when dialog closes
    connect(m_cameraDialog, &QDialog::finished, this, [&]() {
        if (m_camera) {
            m_camera->stop();
        }
    });

    m_cameraDialog->exec();
}

// ── Camera Capture Done ──────────────────────────────────────────────────────
void MainWindow::onCameraCaptureDone(int /*id*/, const QImage &image) {
    if (m_camera) m_camera->stop();
    if (m_cameraDialog) m_cameraDialog->accept();

    m_logView->append("[" + QDateTime::currentDateTime().toString("hh:mm:ss")
                      + "] Camera capture received");

    // Show preview
    QPixmap px = QPixmap::fromImage(image);
    m_previewLabel->setPixmap(px.scaled(
        m_previewLabel->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation));

    // Save captured image to temp for reference
    QString tempPath = QStandardPaths::writableLocation(
                           QStandardPaths::TempLocation)
                       + "/invoice_capture.png";
    image.save(tempPath);
    m_logView->append("  → Saved capture to: " + tempPath);

    // Process via OCR
    processImage(image);
}

// ── Camera Error ─────────────────────────────────────────────────────────────
void MainWindow::onCameraError(int /*id*/, QImageCapture::Error /*error*/,
                               const QString &errorString) {
    m_logView->append("  ✗ Camera error: " + errorString);
    QMessageBox::warning(this, "Camera Error", errorString);
}

// ── Process Image from File ──────────────────────────────────────────────────
void MainWindow::onScanImage() {
    QString path = QFileDialog::getOpenFileName(
        this, "Open Invoice Image", "",
        "Images (*.png *.jpg *.jpeg *.bmp *.tiff *.tif)");
    if (path.isEmpty()) return;
    processImage(path);
}

void MainWindow::processImage(const QString &path) {
    QPixmap px(path);
    if (!px.isNull()) {
        m_previewLabel->setPixmap(px.scaled(
            m_previewLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
    }
    m_progress->setVisible(true);
    m_logView->append("[" + QDateTime::currentDateTime().toString("hh:mm:ss")
                      + "] Scanning: " + path);
    QApplication::processEvents();

    if (!m_ocr.isReady()) {
        QMessageBox::warning(this, "OCR", "OCR engine not ready.");
        m_progress->setVisible(false);
        return;
    }

    QString rawText = m_ocr.extractText(path);
    finishProcessing(rawText);
}

// ── Process QImage (from camera) ─────────────────────────────────────────────
void MainWindow::processImage(const QImage &image) {
    m_progress->setVisible(true);
    m_logView->append("  → Running OCR on captured image...");
    QApplication::processEvents();

    if (!m_ocr.isReady()) {
        QMessageBox::warning(this, "OCR", "OCR engine not ready.");
        m_progress->setVisible(false);
        return;
    }

    QString rawText = m_ocr.extractText(image);
    finishProcessing(rawText);
}

// ── Shared OCR finish logic ──────────────────────────────────────────────────
void MainWindow::finishProcessing(const QString &rawText) {
    if (rawText.trimmed().isEmpty()) {
        m_logView->append("  ⚠ No text extracted from image.");
        m_progress->setVisible(false);
        return;
    }

    m_logView->append("  → Parsing invoice fields...");
    QApplication::processEvents();
    InvoiceData data = m_parser.parse(rawText);

    m_logView->append("  → Saving to database...");
    if (!m_db.saveInvoice(data)) {
        m_logView->append("  ✗ DB error: " + m_db.lastError());
    } else {
        m_logView->append("  ✔ Saved — ID: "    + QString::number(data.dbId)
                          + "  Invoice#: "       + data.invoiceNumber
                          + "  Total: $"         + QString::number(data.totalAmount,'f',2));
        refreshTable();
    }
    m_progress->setVisible(false);
    statusBar()->showMessage("Last scan: "
                             + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
}

// ── Table / CRUD ─────────────────────────────────────────────────────────────
void MainWindow::refreshTable() {
    m_currentList = m_db.loadAllInvoices();
    loadInvoiceToTable(m_currentList);
    statusBar()->showMessage("Loaded " + QString::number(m_currentList.size()) + " invoices");
}

void MainWindow::loadInvoiceToTable(const QList<InvoiceData> &list) {
    m_table->setSortingEnabled(false);
    m_table->setRowCount(0);
    for (const InvoiceData &d : list) {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(d.dbId)));
        m_table->setItem(row, 1, new QTableWidgetItem(d.invoiceNumber));
        m_table->setItem(row, 2, new QTableWidgetItem(d.invoiceDate.toString("yyyy-MM-dd")));
        m_table->setItem(row, 3, new QTableWidgetItem(d.fromName));
        m_table->setItem(row, 4, new QTableWidgetItem(d.toName));
        auto *totalItem = new QTableWidgetItem();
        totalItem->setData(Qt::DisplayRole, d.totalAmount);
        m_table->setItem(row, 5, totalItem);
    }
    m_table->setSortingEnabled(true);
}

int MainWindow::selectedInvoiceDbId() {
    int row = m_table->currentRow();
    if (row < 0) return -1;
    auto *it = m_table->item(row, 0);
    return it ? it->text().toInt() : -1;
}

void MainWindow::onViewInvoice() {
    int id = selectedInvoiceDbId();
    if (id < 0) { QMessageBox::information(this,"","Select an invoice first."); return; }
    InvoiceData data = m_db.loadInvoice(id);
    InvoiceViewDialog dlg(data, false, this);
    dlg.exec();
}

void MainWindow::onEditInvoice() {
    int id = selectedInvoiceDbId();
    if (id < 0) { QMessageBox::information(this,"","Select an invoice first."); return; }
    InvoiceData data = m_db.loadInvoice(id);
    InvoiceViewDialog dlg(data, true, this);
    if (dlg.exec() == QDialog::Accepted) {
        InvoiceData updated = dlg.getEditedData();
        if (!m_db.updateInvoice(updated))
            QMessageBox::critical(this, "Error", m_db.lastError());
        else refreshTable();
    }
}

void MainWindow::onDeleteInvoice() {
    int id = selectedInvoiceDbId();
    if (id < 0) { QMessageBox::information(this,"","Select an invoice first."); return; }
    auto res = QMessageBox::question(this, "Delete",
                                     "Delete invoice ID " + QString::number(id) + "?",
                                     QMessageBox::Yes | QMessageBox::No);
    if (res == QMessageBox::Yes) {
        if (!m_db.deleteInvoice(id))
            QMessageBox::critical(this, "Error", m_db.lastError());
        else refreshTable();
    }
}

void MainWindow::onSearch() {
    QString kw = m_searchBox->text().trimmed();
    if (kw.isEmpty()) { refreshTable(); return; }
    loadInvoiceToTable(m_db.searchInvoices(kw));
    statusBar()->showMessage("Search results for: " + kw);
}

void MainWindow::onClearSearch() {
    m_searchBox->clear();
    refreshTable();
}

void MainWindow::onTableDoubleClick(int /*row*/, int /*col*/) {
    onViewInvoice();
}

void MainWindow::onExportCSV() {
    QString path = QFileDialog::getSaveFileName(
        this, "Export CSV", "invoices_export.csv", "CSV (*.csv)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot write file.");
        return;
    }
    QTextStream out(&f);
    out << "DB ID,Invoice #,Date,From,To,Subtotal,Tax Rate,Tax Amount,Total\n";
    for (const InvoiceData &d : m_currentList) {
        out << d.dbId << ","
            << "\"" << d.invoiceNumber << "\","
            << d.invoiceDate.toString("yyyy-MM-dd") << ","
            << "\"" << d.fromName << "\","
            << "\"" << d.toName  << "\","
            << d.subtotal    << ","
            << d.taxRate     << ","
            << d.taxAmount   << ","
            << d.totalAmount << "\n";
    }
    f.close();
    m_logView->append("✔ Exported to: " + path);
}

// ── Stylesheet ───────────────────────────────────────────────────────────────
void MainWindow::applyStyleSheet()
{
    setStyleSheet(R"(
        QWidget {
            background:#f5f5f2;
            color:#1e1e1e;
            font-family:"Segoe UI","SF Pro Display","Helvetica Neue",sans-serif;
            font-size:12px;
        }

        QMainWindow {
            background:#f5f5f2;
        }

        QPushButton {
            background:#ffffff;
            color:#1e1e1e;
            border:1px solid #d8cfc8;
            border-radius:6px;
            padding:6px 14px;
            font-size:12px;
        }

        QPushButton:hover {
            background:#fff3ec;
            border-color:#eb5f14;
            color:#eb5f14;
        }

        QPushButton:pressed {
            background:#ffe8d8;
        }

        QPushButton:disabled {
            color:#b8b0a8;
            border-color:#e8e2dc;
            background:#f8f6f4;
        }

        QLineEdit {
            background:#ffffff;
            border:1px solid #d8cfc8;
            border-radius:6px;
            padding:6px 10px;
            color:#1e1e1e;
        }

        QLineEdit:focus {
            border-color:#eb5f14;
        }

        QTableWidget {
            background:#ffffff;
            border:1px solid #e0d8d0;
            border-radius:6px;
            gridline-color:#eee8e2;
            selection-background-color:#fff0e8;
            selection-color:#eb5f14;
        }

        QHeaderView::section {
            background:#fffaf6;
            color:#6e6460;
            padding:6px;
            border:none;
            font-weight:600;
        }

        QTableWidget::item:alternate {
            background:#faf7f4;
        }

        QGroupBox {
            border:1px solid #e0d8d0;
            border-radius:6px;
            margin-top:8px;
            padding:8px;
            font-weight:600;
            color:#6e6460;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
        }

        QTextEdit {
            background:#ffffff;
            border:1px solid #e0d8d0;
            border-radius:6px;
            color:#1e1e1e;
        }

        QComboBox {
            background:#ffffff;
            border:1px solid #d8cfc8;
            border-radius:6px;
            padding:4px 8px;
            color:#1e1e1e;
        }

        QComboBox:hover {
            border-color:#eb5f14;
        }

        QComboBox QAbstractItemView {
            background:#ffffff;
            color:#1e1e1e;
            selection-background-color:#fff0e8;
            selection-color:#eb5f14;
        }

        QProgressBar {
            border:none;
            border-radius:4px;
            background:#f0ece8;
            height:8px;
        }

        QProgressBar::chunk {
            background:#eb5f14;
            border-radius:4px;
        }

        QMenuBar {
            background:#ffffff;
            color:#1e1e1e;
        }

        QMenuBar::item:selected {
            background:#fff0e8;
            color:#eb5f14;
        }

        QMenu {
            background:#ffffff;
            color:#1e1e1e;
            border:1px solid #e0d8d0;
        }

        QMenu::item:selected {
            background:#fff0e8;
            color:#eb5f14;
        }

        QToolBar {
            background:#ffffff;
            border:none;
            spacing:4px;
            padding:4px;
        }

        QStatusBar {
            background:#ffffff;
            color:#6e6460;
        }

        QScrollBar:vertical {
            background:#f5f5f2;
            width:10px;
        }

        QScrollBar::handle:vertical {
            background:#d8cfc8;
            border-radius:5px;
        }

        QDialog {
            background:#f5f5f2;
        }
    )");
}
