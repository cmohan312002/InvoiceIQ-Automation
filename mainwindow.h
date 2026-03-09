#pragma once
#include <QMainWindow>
#include "databasemanager.h"
#include "ocrengine.h"
#include "invoiceparser.h"
#include "invoicedata.h"

// Camera
#include <QCamera>
#include <QMediaCaptureSession>
#include <QImageCapture>
#include <QVideoWidget>

class QTableWidget;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QStatusBar;
class QSplitter;
class QTextEdit;
class QDialog;

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

    // Camera slots
    void onCameraCaptureDone(int id, const QImage &image);
    void onCameraError(int id, QImageCapture::Error error, const QString &errorString);

private:
    void buildUI();
    void buildMenuBar();
    void buildToolBar();
    void applyStyleSheet();
    void loadInvoiceToTable(const QList<InvoiceData> &list);
    void processImage(const QString &path);
    void processImage(const QImage &image);   // overload for camera
    int  selectedInvoiceDbId();
    void finishProcessing(const QString &rawText);

    // Main widgets
    QTableWidget *m_table;
    QLineEdit    *m_searchBox;
    QLabel       *m_previewLabel;
    QProgressBar *m_progress;
    QTextEdit    *m_logView;
    QPushButton  *m_scanBtn, *m_viewBtn, *m_editBtn, *m_deleteBtn;

    // Camera
    QCamera               *m_camera        = nullptr;
    QMediaCaptureSession  *m_captureSession = nullptr;
    QImageCapture         *m_imageCapture   = nullptr;
    QVideoWidget          *m_videoWidget    = nullptr;
    QDialog               *m_cameraDialog   = nullptr;

    // Backend
    DatabaseManager    m_db;
    OcrEngine          m_ocr;
    InvoiceParser      m_parser;
    QList<InvoiceData> m_currentList;
};
