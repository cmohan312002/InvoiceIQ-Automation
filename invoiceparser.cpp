#include "invoiceparser.h"
#include <QRegularExpression>
#include <QStringList>
#include <QDebug>

InvoiceParser::InvoiceParser() {}

InvoiceData InvoiceParser::parse(const QString &rawText) {
    InvoiceData data;
    data.rawText = rawText;

    // Invoice number
    {
        QRegularExpression re(R"(invoice\s*#?\s*[:\-]?\s*([A-Z0-9\-]+))",
                              QRegularExpression::CaseInsensitiveOption);
        auto m = re.match(rawText);
        if (m.hasMatch()) data.invoiceNumber = m.captured(1).trimmed();
    }

    // Date
    data.invoiceDate = parseDate(rawText);

    // FROM block
    {
        QRegularExpression re(R"(from\s*[:\n](.*?)(?=to\s*[:\n]|$))",
                              QRegularExpression::CaseInsensitiveOption |
                                  QRegularExpression::DotMatchesEverythingOption);
        auto m = re.match(rawText);
        if (m.hasMatch()) {
            QString block = m.captured(1);
            QStringList lines = block.split('\n', Qt::SkipEmptyParts);
            if (!lines.isEmpty()) data.fromName = lines[0].trimmed();

            QRegularExpression addrRe(R"(address\s*[:\-]?\s*(.+))",
                                      QRegularExpression::CaseInsensitiveOption);
            auto am = addrRe.match(block);
            if (am.hasMatch()) data.fromAddress = am.captured(1).trimmed();

            QRegularExpression emailRe(R"(email\s*(?:address)?\s*[:\-]?\s*([\w.\-+]+@[\w.\-]+))",
                                       QRegularExpression::CaseInsensitiveOption);
            auto em = emailRe.match(block);
            if (em.hasMatch()) data.fromEmail = em.captured(1).trimmed();

            QRegularExpression phoneRe(R"(phone\s*(?:number)?\s*[:\-]?\s*([\+\d\s\(\)\-]{7,20}))",
                                       QRegularExpression::CaseInsensitiveOption);
            auto pm = phoneRe.match(block);
            if (pm.hasMatch()) data.fromPhone = pm.captured(1).trimmed();
        }
    }

    // TO block
    {
        QRegularExpression re(R"(to\s*[:\n](.*?)(?=item|#\s*item|description|qty|$))",
                              QRegularExpression::CaseInsensitiveOption |
                                  QRegularExpression::DotMatchesEverythingOption);
        auto m = re.match(rawText);
        if (m.hasMatch()) {
            QString block = m.captured(1);
            QStringList lines = block.split('\n', Qt::SkipEmptyParts);
            if (!lines.isEmpty()) data.toName = lines[0].trimmed();

            QRegularExpression addrRe(R"((?:attn|address)\s*[:\-]?\s*(.+))",
                                      QRegularExpression::CaseInsensitiveOption);
            auto am = addrRe.match(block);
            if (am.hasMatch()) data.toAddress = am.captured(1).trimmed();

            QRegularExpression emailRe(R"(email\s*(?:address)?\s*[:\-]?\s*([\w.\-+]+@[\w.\-]+))",
                                       QRegularExpression::CaseInsensitiveOption);
            auto em = emailRe.match(block);
            if (em.hasMatch()) data.toEmail = em.captured(1).trimmed();

            QRegularExpression phoneRe(R"(phone\s*(?:number)?\s*[:\-]?\s*([\+\d\s\(\)\-]{7,20}))",
                                       QRegularExpression::CaseInsensitiveOption);
            auto pm = phoneRe.match(block);
            if (pm.hasMatch()) data.toPhone = pm.captured(1).trimmed();
        }
    }

    // Line items
    data.items = parseItems(rawText);

    // Subtotal
    {
        QRegularExpression re(R"(subtotal\s*[:\$]?\s*([\d,]+\.?\d*))",
                              QRegularExpression::CaseInsensitiveOption);
        auto m = re.match(rawText);
        if (m.hasMatch()) data.subtotal = parseAmount(m.captured(1));
    }

    // Tax rate
    {
        QRegularExpression re(R"(tax\s*rate\s*[:\-]?\s*([\d.]+)\s*%?\s*\$?\s*([\d,]+\.?\d*))",
                              QRegularExpression::CaseInsensitiveOption);
        auto m = re.match(rawText);
        if (m.hasMatch()) {
            data.taxRate   = m.captured(1).toDouble();
            data.taxAmount = parseAmount(m.captured(2));
        }
    }

    // Total
    {
        QRegularExpression re(R"(total\s*\n.*?\$\s*([\d,]+\.?\d*))",
                              QRegularExpression::CaseInsensitiveOption |
                                  QRegularExpression::DotMatchesEverythingOption);
        auto m = re.match(rawText);
        if (m.hasMatch()) data.totalAmount = parseAmount(m.captured(1));
    }

    // Payment note
    {
        QRegularExpression re(R"(payment\s*instructions?\s*[:\n](.*?)(?=signature|$))",
                              QRegularExpression::CaseInsensitiveOption |
                                  QRegularExpression::DotMatchesEverythingOption);
        auto m = re.match(rawText);
        if (m.hasMatch()) data.paymentNote = m.captured(1).trimmed().replace('\n', ' ');
    }

    return data;
}

QDate InvoiceParser::parseDate(const QString &text) {
    // e.g. "15 October 2023" or "2023-10-15" or "10/15/2023"
    QStringList formats = {
        "d MMMM yyyy", "dd MMMM yyyy",
        "MMMM d, yyyy", "MMMM dd, yyyy",
        "yyyy-MM-dd", "MM/dd/yyyy", "dd/MM/yyyy"
    };
    QRegularExpression re(
        R"((\d{1,2}\s+\w+\s+\d{4}|\d{4}-\d{2}-\d{2}|\d{1,2}/\d{1,2}/\d{4}|\w+\s+\d{1,2},\s*\d{4}))",
        QRegularExpression::CaseInsensitiveOption);
    auto m = re.match(text);
    if (m.hasMatch()) {
        QString ds = m.captured(1).trimmed();
        for (const QString &fmt : formats) {
            QDate d = QDate::fromString(ds, fmt);
            if (d.isValid()) return d;
        }
    }
    return QDate();
}

QList<InvoiceItem> InvoiceParser::parseItems(const QString &text) {
    QList<InvoiceItem> items;
    // Match lines like: "1  Do It Yourself Tornado Kit  1  $ 489.00  $ 489.00"
    QRegularExpression re(
        R"((\d+)\s+([A-Za-z][^\$\n]{3,50?})\s+(\d+(?:\.\d+)?)\s+\$?\s*([\d,]+\.?\d*)\s+(?:([\d.]+%?)\s+)?\$?\s*([\d,]+\.?\d*))",
        QRegularExpression::CaseInsensitiveOption);

    auto it = re.globalMatch(text);
    while (it.hasNext()) {
        auto m = it.next();
        InvoiceItem item;
        item.itemId   = m.captured(1).toInt();
        item.itemName = m.captured(2).trimmed();
        item.qty      = m.captured(3).toDouble();
        item.unitCost = parseAmount(m.captured(4));
        QString discStr = m.captured(5);
        item.discount = discStr.isEmpty() ? 0.0 : parsePercent(discStr);
        item.total    = parseAmount(m.captured(6));
        if (!item.itemName.isEmpty() && item.qty > 0)
            items.append(item);
    }
    return items;
}

double InvoiceParser::parseAmount(const QString &text) {
    QString clean = text;
    clean.remove(',').remove('$').remove('%').trimmed();
    return clean.toDouble();
}

double InvoiceParser::parsePercent(const QString &text) {
    QString clean = text;
    clean.remove('%').trimmed();
    return clean.toDouble();
}

QString InvoiceParser::cleanValue(const QString &raw) {
    return raw.trimmed().replace(QRegularExpression(R"(\s{2,})"), " ");
}
