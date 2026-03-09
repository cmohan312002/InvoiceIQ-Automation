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
#include <QCamera>
#include <QImageCapture>
#include <QMediaCaptureSession>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Invoice Scanner — Aether Solutions");
    setMinimumSize(1200, 750);
    applyStyleSheet();
    buildMenuBar();
    buildToolBar();
    buildUI();

    // Init OCR
    if (!m_ocr.initialize()) {
        statusBar()->showMessage("⚠ Tesseract not found — check tessdata path");
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

MainWindow::~MainWindow() {}

// ── UI Construction ──────────────────────────────────────────────────────────

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
    connect(searchBtn,  &QPushButton::clicked, this, &MainWindow::onSearch);
    connect(clearBtn,   &QPushButton::clicked, this, &MainWindow::onClearSearch);
    connect(m_searchBox,&QLineEdit::returnPressed, this, &MainWindow::onSearch);

    // Main splitter
    QSplitter *splitter = new QSplitter(Qt::Horizontal);

    // Left: table + actions
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
    m_table->setColumnWidth(0, 60);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &MainWindow::onTableDoubleClick);

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

    // Right: preview + log
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

    QGroupBox *logGrp = new QGroupBox("Activity Log");
    QVBoxLayout *lgl = new QVBoxLayout(logGrp);
    m_logView = new QTextEdit;
    m_logView->setReadOnly(true);
    m_logView->setFont(QFont("Courier New", 9));
    m_logView->setMaximumHeight(180);
    lgl->addWidget(m_logView);

    m_progress = new QProgressBar;
    m_progress->setVisible(false);
    m_progress->setRange(0,0);

    rv->addWidget(prevGrp);
    rv->addWidget(logGrp);
    rv->addWidget(m_progress);
    rv->addStretch();

    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    mainLayout->addLayout(searchRow);
    mainLayout->addWidget(splitter);
}

void MainWindow::buildMenuBar() {
    QMenuBar *mb = menuBar();

    // ── File Menu ─────────────────────────────────────────────────
    QMenu *fileMenu = mb->addMenu("&File");

    QAction *scanImgAct = new QAction("Scan Image", this);
    scanImgAct->setShortcut(QKeySequence("Ctrl+O"));
    connect(scanImgAct, &QAction::triggered, this, &MainWindow::onScanImage);
    fileMenu->addAction(scanImgAct);

    QAction *scanCamAct = new QAction("Scan Camera", this);
    scanCamAct->setShortcut(QKeySequence("Ctrl+K"));
    connect(scanCamAct, &QAction::triggered, this, &MainWindow::onScanCamera);
    fileMenu->addAction(scanCamAct);

    fileMenu->addSeparator();

    QAction *exportAct = new QAction("Export CSV", this);
    exportAct->setShortcut(QKeySequence("Ctrl+E"));
    connect(exportAct, &QAction::triggered, this, &MainWindow::onExportCSV);
    fileMenu->addAction(exportAct);

    fileMenu->addSeparator();

    QAction *exitAct = new QAction("Exit", this);
    exitAct->setShortcut(QKeySequence("Ctrl+Q"));
    connect(exitAct, &QAction::triggered, qApp, &QApplication::quit);
    fileMenu->addAction(exitAct);

    // ── Invoice Menu ──────────────────────────────────────────────
    QMenu *invoiceMenu = mb->addMenu("&Invoice");

    QAction *viewAct = new QAction("View", this);
    connect(viewAct, &QAction::triggered, this, &MainWindow::onViewInvoice);
    invoiceMenu->addAction(viewAct);

    QAction *editAct = new QAction("Edit", this);
    connect(editAct, &QAction::triggered, this, &MainWindow::onEditInvoice);
    invoiceMenu->addAction(editAct);

    QAction *delAct = new QAction("Delete", this);
    connect(delAct, &QAction::triggered, this, &MainWindow::onDeleteInvoice);
    invoiceMenu->addAction(delAct);

    QAction *refreshAct = new QAction("Refresh", this);
    connect(refreshAct, &QAction::triggered, this, &MainWindow::refreshTable);
    invoiceMenu->addAction(refreshAct);

    // ── Help Menu ─────────────────────────────────────────────────
    QMenu *helpMenu = mb->addMenu("&Help");

    QAction *aboutAct = new QAction("About", this);
    connect(aboutAct, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "About",
                           "<b>Invoice Scanner</b><br>v1.0<br><br>"
                           "OCR-powered invoice digitizer<br>"
                           "Built with Qt + Tesseract + SQLite");
    });
    helpMenu->addAction(aboutAct);
}

void MainWindow::buildToolBar() {
    QToolBar *tb = addToolBar("Main");
    tb->setMovable(false);

    QAction *a1 = new QAction("Scan Image", this);
    connect(a1, &QAction::triggered, this, &MainWindow::onScanImage);
    tb->addAction(a1);

    QAction *a2 = new QAction("Scan Camera", this);
    connect(a2, &QAction::triggered, this, &MainWindow::onScanCamera);
    tb->addAction(a2);

    tb->addSeparator();

    QAction *a3 = new QAction("Refresh", this);
    connect(a3, &QAction::triggered, this, &MainWindow::refreshTable);
    tb->addAction(a3);

    QAction *a4 = new QAction("Export CSV", this);
    connect(a4, &QAction::triggered, this, &MainWindow::onExportCSV);
    tb->addAction(a4);
}
void MainWindow::applyStyleSheet() {
    setStyleSheet(R"(
        QMainWindow { background: #13131f; }
        QWidget     { background: #1a1a2e; color: #e0e0e0; font-family: Segoe UI; font-size: 13px; }
        QGroupBox   { border: 1px solid #3a3a5c; border-radius: 6px; margin-top: 8px; padding: 8px;
                      font-weight: bold; color: #a0c4ff; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; }
        QTableWidget { background: #12122a; gridline-color: #2a2a4a;
                       selection-background-color: #4a4aaa; border: none; border-radius:4px; }
        QHeaderView::section { background: #252550; color: #a0c4ff;
                               padding: 6px; border: none; font-weight:bold; }
        QTableWidget::item:alternate { background: #16163a; }
        QLineEdit   { background: #252545; border: 1px solid #4a4a7a; border-radius:4px;
                      padding:4px 8px; color:#e0e0e0; }
        QLineEdit:focus { border-color: #7a7aff; }
        QPushButton { background: #3a3a8a; color: #ffffff; border: none;
                      border-radius: 5px; padding: 6px 14px; font-weight:bold; }
        QPushButton:hover  { background: #5a5acc; }
        QPushButton:pressed{ background: #2a2a6a; }
        QTextEdit   { background: #12122a; border: 1px solid #3a3a5c;
                      border-radius: 4px; color: #c8c8d8; }
        QTabWidget::pane { border: 1px solid #3a3a5c; border-radius:4px; }
        QTabBar::tab { background:#252545; color:#a0a0c0; padding:7px 16px;
                       border-radius:4px 4px 0 0; margin-right:2px; }
        QTabBar::tab:selected { background:#3a3a8a; color:#ffffff; }
        QProgressBar { border:none; border-radius:4px; background:#252545; height:8px; }
        QProgressBar::chunk { background:#7a7aff; border-radius:4px; }
        QMenuBar { background:#111128; color:#e0e0e0; }
        QMenuBar::item:selected { background:#3a3a8a; }
        QMenu { background:#1a1a3a; color:#e0e0e0; border:1px solid #3a3a5c; }
        QMenu::item:selected { background:#3a3a8a; }
        QToolBar { background:#111128; border:none; spacing:4px; padding:2px; }
        QScrollBar:vertical { background:#1a1a2e; width:10px; }
        QScrollBar::handle:vertical { background:#3a3a6a; border-radius:5px; }
        QDoubleSpinBox { background:#252545; border:1px solid #4a4a7a; border-radius:4px;
                         padding:4px; color:#e0e0e0; }
        QDateEdit { background:#252545; border:1px solid #4a4a7a; border-radius:4px;
                    padding:4px; color:#e0e0e0; }
        QStatusBar { background:#111128; color:#888; }
    )");
}

// ── Slots ────────────────────────────────────────────────────────────────────

void MainWindow::onScanImage() {
    QString path = QFileDialog::getOpenFileName(
        this, "Open Invoice Image", "",
        "Images (*.png *.jpg *.jpeg *.bmp *.tiff *.tif *.webp)");
    if (path.isEmpty()) return;
    processImage(path);
}

void MainWindow::onScanCamera() {
    QMessageBox::information(this, "Camera",
                             "Camera capture: use your OS camera app to take a photo,\n"
                             "then use 'Scan Image' to load the saved file.\n\n"
                             "(Integrate QCamera for live capture in production builds.)");
}

void MainWindow::processImage(const QString &path) {
    // Show preview
    QPixmap px(path);
    if (!px.isNull()) {
        m_previewLabel->setPixmap(px.scaled(
            m_previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    m_progress->setVisible(true);
    m_logView->append("[" + QDateTime::currentDateTime().toString("hh:mm:ss") + "] Loading: " + path);
    QApplication::processEvents();

    if (!m_ocr.isReady()) {
        QMessageBox::warning(this, "OCR", "OCR engine not ready. Check Tesseract installation.");
        m_progress->setVisible(false);
        return;
    }

    m_logView->append("  → Running OCR...");
    QApplication::processEvents();
    QString rawText = m_ocr.extractText(path);

    if (rawText.trimmed().isEmpty()) {
        m_logView->append("  ⚠ No text extracted.");
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
        m_logView->append("  ✔ Saved — ID: " + QString::number(data.dbId)
                          + "  Invoice#: " + data.invoiceNumber
                          + "  Total: $" + QString::number(data.totalAmount,'f',2));
        refreshTable();
    }

    m_progress->setVisible(false);
    statusBar()->showMessage("Last scan: " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
}

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
    QList<InvoiceData> results = m_db.searchInvoices(kw);
    loadInvoiceToTable(results);
    statusBar()->showMessage("Found " + QString::number(results.size()) + " result(s)");
}

void MainWindow::onClearSearch() {
    m_searchBox->clear();
    refreshTable();
}

void MainWindow::onTableDoubleClick(int /*row*/, int /*col*/) {
    onViewInvoice();
}

void MainWindow::onExportCSV() {
    QString path = QFileDialog::getSaveFileName(this,"Export CSV","invoices_export.csv","CSV (*.csv)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this,"Error","Cannot write file.");
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
    statusBar()->showMessage("CSV exported: " + path);
}
