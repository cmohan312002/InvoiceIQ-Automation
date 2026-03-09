#pragma once
#include <QMainWindow>
#include "databasemanager.h"
#include "ocrengine.h"
#include "invoiceparser.h"
#include "invoicedata.h"

class QTableWidget;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QStatusBar;
class QSplitter;
class QTextEdit;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onScanImage();
    void onScanCamera();
    void onViewInvoice();
    void onEditInvoice();
    void onDeleteInvoice();
    void onSearch();
    void onClearSearch();
    void onExportCSV();
    void onTableDoubleClick(int row, int col);
    void refreshTable();

private:
    void buildUI();
    void buildMenuBar();
    void buildToolBar();
    void applyStyleSheet();
    void loadInvoiceToTable(const QList<InvoiceData> &list);
    void processImage(const QString &path);
    int  selectedInvoiceDbId();

    // Widgets
    QTableWidget *m_table;
    QLineEdit    *m_searchBox;
    QLabel       *m_previewLabel;
    QProgressBar *m_progress;
    QTextEdit    *m_logView;
    QPushButton  *m_scanBtn, *m_viewBtn, *m_editBtn, *m_deleteBtn;

    // Backend
    DatabaseManager m_db;
    OcrEngine       m_ocr;
    InvoiceParser   m_parser;

    QList<InvoiceData> m_currentList;
};
