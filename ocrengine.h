#pragma once
#include <QString>
#include <QImage>

// vcpkg MinGW layout
#include <tesseract/baseapi.h>
#include <tesseract/publictypes.h>
#include <leptonica/allheaders.h>

class OcrEngine {
public:
    OcrEngine();
    ~OcrEngine();

    bool    initialize(const QString &tessDataPath = "");
    QString extractText(const QString &imagePath);
    QString extractText(const QImage &image);
    bool    isReady()    const { return m_ready; }
    QString lastError()  const { return m_lastError; }

private:
    tesseract::TessBaseAPI *m_api = nullptr;
    bool    m_ready     = false;
    QString m_lastError;

    Pix* qImageToPix(const QImage &image);
};
