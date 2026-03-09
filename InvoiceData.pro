QT       += core gui sql widgets multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET   = InvoiceData
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    ocrengine.cpp \
    invoiceparser.cpp \
    databasemanager.cpp \
    invoiceviewdialog.cpp

HEADERS += \
    mainwindow.h \
    ocrengine.h \
    invoiceparser.h \
    databasemanager.h \
    invoicedata.h \
    invoiceviewdialog.h

# ── vcpkg base path ──────────────────────────────────────────────
VCPKG_DIR = D:/Workspace/Mohan/vcpkg/installed/x64-mingw-dynamic

# ── Include paths ────────────────────────────────────────────────
INCLUDEPATH += $$VCPKG_DIR/include
INCLUDEPATH += $$VCPKG_DIR/include/tesseract
INCLUDEPATH += $$VCPKG_DIR/include/leptonica

# ── Library search path ──────────────────────────────────────────
LIBS += -L$$VCPKG_DIR/lib

# ── Tesseract  →  libtesseract55.dll.a ───────────────────────────
LIBS += -ltesseract55

# ── Leptonica  →  libleptonica.dll.a ─────────────────────────────
LIBS += -lleptonica

# ── Leptonica dependencies (all confirmed in your lib folder) ────
LIBS += -lpng16
LIBS += -ljpeg
LIBS += -ltiff
LIBS += -lwebp
LIBS += -lwebpdecoder
LIBS += -lwebpdemux
LIBS += -lwebpmux
LIBS += -lzlib
LIBS += -lsharpyuv
LIBS += -lgif
LIBS += -lopenjp2
LIBS += -llzma
LIBS += -lbz2

# ── SQLite  →  bundled inside Qt (qsqlite.dll) ───────────────────
# No -lsqlite3 needed — Qt handles it via QSQLITE driver

# ── Suppress known MinGW/Qt6 warnings ───────────────────────────
QMAKE_CXXFLAGS += -Wno-deprecated-declarations
QMAKE_CXXFLAGS += -Wno-unused-parameter
QMAKE_CXXFLAGS += -Wno-unused-variable
QMAKE_CXXFLAGS += -Wno-unused-result

# ── Runtime DLL note ─────────────────────────────────────────────
# Add to Qt Creator → Projects → Run → Environment:
# PATH = D:\Workspace\Mohan\vcpkg\installed\x64-mingw-dynamic\bin;%PATH%
#
# Required DLLs at runtime:
#   libtesseract55.dll
#   libleptonica-1.87.0.dll
#
# tessdata location (confirmed 23MB):
#   D:\Workspace\Mohan\vcpkg\installed\x64-mingw-dynamic\share\tessdata\eng.traineddata

# ── Install (optional) ───────────────────────────────────────────
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
```
