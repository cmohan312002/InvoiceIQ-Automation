#include "ocrengine.h"
#include <QDebug>
#include <QFileInfo>
#include <QCoreApplication>
OcrEngine::OcrEngine() {}

OcrEngine::~OcrEngine() {
    if (m_api) {
        m_api->End();
        delete m_api;
        m_api = nullptr;
    }
}

// bool OcrEngine::initialize(const QString &tessDataPath) {
//     m_api = new tesseract::TessBaseAPI();

//     // Try caller-supplied path first, then vcpkg share, then env var
//     QStringList candidates;
//     if (!tessDataPath.isEmpty())
//         candidates << tessDataPath;

//     // vcpkg installs tessdata here:
//     candidates << "D:/Workspace/Mohan/vcpkg/installed/x64-mingw-dynamic/share/tessdata";
//     candidates << "D:/Workspace/Mohan/vcpkg/installed/x64-mingw-dynamic/share/tesseract";

//     // Also respect TESSDATA_PREFIX env var if set
//     QString envPath = qEnvironmentVariable("TESSDATA_PREFIX");
//     if (!envPath.isEmpty()) candidates.prepend(envPath);

//     for (const QString &path : candidates) {
//         QFileInfo fi(path);
//         if (!fi.exists()) continue;

//         if (m_api->Init(path.toUtf8().constData(), "eng") == 0) {
//             m_api->SetPageSegMode(tesseract::PSM_AUTO);
//             m_ready = true;
//             qDebug() << "[OCR] Initialized with tessdata:" << path;
//             return true;
//         }
//     }

//     m_lastError = "Tesseract init failed. No valid tessdata found in:\n"
//                   + candidates.join("\n");
//     m_ready = false;
//     return false;
// }

bool OcrEngine::initialize(const QString &tessDataPath)
{
    m_api = new tesseract::TessBaseAPI();

    // 🔥 THIS IS THE FIX
    QString appPath = QCoreApplication::applicationDirPath() + "/tessdata";

    if (m_api->Init(appPath.toUtf8().constData(), "eng") == 0)
    {
        m_api->SetPageSegMode(tesseract::PSM_AUTO);
        m_ready = true;
        qDebug() << "✅ OCR working from:" << appPath;
        return true;
    }

    m_lastError = "Tesseract init failed";
    m_ready = false;
    return false;
}

QString OcrEngine::extractText(const QString &imagePath) {
    if (!m_ready) { m_lastError = "OCR not initialized"; return ""; }

    Pix *pix = pixRead(imagePath.toUtf8().constData());
    if (!pix) {
        m_lastError = "Failed to load image: " + imagePath;
        return "";
    }

    m_api->SetImage(pix);
    m_api->Recognize(nullptr);

    char *outText = m_api->GetUTF8Text();
    QString result = QString::fromUtf8(outText);
    delete[] outText;
    pixDestroy(&pix);
    return result;
}

QString OcrEngine::extractText(const QImage &image) {
    if (!m_ready) { m_lastError = "OCR not initialized"; return ""; }

    Pix *pix = qImageToPix(image);
    if (!pix) {
        m_lastError = "QImage to Pix conversion failed";
        return "";
    }

    m_api->SetImage(pix);
    m_api->Recognize(nullptr);

    char *outText = m_api->GetUTF8Text();
    QString result = QString::fromUtf8(outText);
    delete[] outText;
    pixDestroy(&pix);
    return result;
}


    Pix* OcrEngine::qImageToPix(const QImage &image) {
        QImage rgb = image.convertToFormat(QImage::Format_RGB888);
        int w = rgb.width();
        int h = rgb.height();
        // bpl removed — was unused

        Pix *pix = pixCreate(w, h, 32);
        for (int y = 0; y < h; ++y) {
            const uchar *src = rgb.scanLine(y);
            for (int x = 0; x < w; ++x) {
                l_uint32 r = src[x * 3 + 0];
                l_uint32 g = src[x * 3 + 1];
                l_uint32 b = src[x * 3 + 2];
                l_uint32 val = (r << 24) | (g << 16) | (b << 8) | 0xFF;
                pixSetPixel(pix, x, y, val);
            }
        }
        return pix;
    }

