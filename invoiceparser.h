#pragma once
#include "invoicedata.h"
#include <QString>
#include <QStringList>

class InvoiceParser {
public:
    InvoiceParser();
    InvoiceData parse(const QString &rawText);

private:
    // Section extractors
    QString            extractInvoiceNumber(const QStringList &lines);
    QDate              extractDate(const QStringList &lines);
    void               extractFromBlock(const QStringList &lines, InvoiceData &data);
    void               extractToBlock(const QStringList &lines, InvoiceData &data);
    QList<InvoiceItem> extractItems(const QStringList &lines);
    void               extractTotals(const QStringList &lines, InvoiceData &data);
    QString            extractPaymentNote(const QStringList &lines);

    // Helpers
    double  parseAmount(const QString &text);
    double  parsePercent(const QString &text);
    QDate   parseDate(const QString &text);
    bool    isItemLine(const QString &line);
    bool    lineContainsAmount(const QString &line);
    QString cleanLine(const QString &line);
    QString extractEmail(const QString &text);
    QString extractPhone(const QString &text);
    QString extractAddress(const QString &text);
    // QString fixOcrInvoice(QString code);
};
