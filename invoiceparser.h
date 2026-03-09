#pragma once
#include "invoicedata.h"
#include <QString>

class InvoiceParser {
public:
    InvoiceParser();
    InvoiceData parse(const QString &rawText);

private:
    QString     extractField(const QString &text, const QStringList &labels);
    double      parseAmount(const QString &text);
    double      parsePercent(const QString &text);
    QDate       parseDate(const QString &text);
    QList<InvoiceItem> parseItems(const QString &text);
    QString     cleanValue(const QString &raw);
};
