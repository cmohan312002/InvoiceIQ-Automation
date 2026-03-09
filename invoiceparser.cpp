#include "invoiceparser.h"
#include <QRegularExpression>
#include <QDebug>

InvoiceParser::InvoiceParser() {}

// ── Main Entry ───────────────────────────────────────────────────────────────
InvoiceData InvoiceParser::parse(const QString &rawText) {
    InvoiceData data;
    data.rawText = rawText;

    // Split into clean lines
    QStringList allLines;
    for (const QString &l : rawText.split('\n')) {
        QString cl = cleanLine(l);
        if (!cl.isEmpty())
            allLines << cl;
    }

    qDebug() << "[Parser] Total lines:" << allLines.size();
    for (int i = 0; i < allLines.size(); i++)
        qDebug() << QString("[%1] %2").arg(i).arg(allLines[i]);

    data.invoiceNumber = extractInvoiceNumber(allLines);
    data.invoiceDate   = extractDate(allLines);

    extractFromBlock(allLines, data);
    extractToBlock(allLines, data);

    data.items = extractItems(allLines);
    extractTotals(allLines, data);
    data.paymentNote = extractPaymentNote(allLines);

    return data;
}

// ── Clean Line ───────────────────────────────────────────────────────────────
QString InvoiceParser::cleanLine(const QString &line) {
    QString cl = line.trimmed();
    // Collapse multiple spaces
    cl.replace(QRegularExpression(R"(\s{2,})"), " ");
    return cl;
}

// ── Invoice Number ───────────────────────────────────────────────────────────
QString InvoiceParser::extractInvoiceNumber(const QStringList &lines) {
    // Patterns: "INVOICE # AES-12345" or "AES-12345" or "INV-123"
    QRegularExpression re1(R"(invoice\s*#?\s*[:\-]?\s*([A-Z]{2,5}[\-]\d+))",
                           QRegularExpression::CaseInsensitiveOption);
    QRegularExpression re2(R"(\b([A-Z]{2,5}-\d{3,})\b)");

    for (const QString &line : lines) {
        auto m = re1.match(line);
        if (m.hasMatch()) return m.captured(1).trimmed();
    }
    // Second pass — look for pattern like AES-12345
    for (const QString &line : lines) {
        auto m = re2.match(line);
        if (m.hasMatch()) return m.captured(1).trimmed();
    }
    return "";
}

// ── Date ─────────────────────────────────────────────────────────────────────
QDate InvoiceParser::extractDate(const QStringList &lines) {
    for (const QString &line : lines) {
        QDate d = parseDate(line);
        if (d.isValid()) return d;
    }
    return QDate();
}

QDate InvoiceParser::parseDate(const QString &text) {
    // "15 October 2023" or "October 15, 2023" or "2023-10-15" or "10/15/2023"
    struct Pattern { QString re; QString fmt; };
    QList<Pattern> patterns = {
                               { R"((\d{1,2}\s+\w+\s+\d{4}))",         "d MMMM yyyy"   },
                               { R"((\d{1,2}\s+\w+\s+\d{4}))",         "dd MMMM yyyy"  },
                               { R"((\w+\s+\d{1,2},?\s+\d{4}))",       "MMMM d, yyyy"  },
                               { R"((\w+\s+\d{1,2},?\s+\d{4}))",       "MMMM dd, yyyy" },
                               { R"((\d{4}-\d{2}-\d{2}))",             "yyyy-MM-dd"    },
                               { R"((\d{1,2}/\d{1,2}/\d{4}))",         "MM/dd/yyyy"    },
                               { R"((\d{1,2}/\d{1,2}/\d{4}))",         "dd/MM/yyyy"    },
                               };
    for (const Pattern &p : patterns) {
        QRegularExpression re(p.re, QRegularExpression::CaseInsensitiveOption);
        auto m = re.match(text);
        if (m.hasMatch()) {
            QString ds = m.captured(1).trimmed().replace(",","");
            QDate d = QDate::fromString(ds, p.fmt);
            if (d.isValid()) return d;
        }
    }
    return QDate();
}

// ── FROM Block ───────────────────────────────────────────────────────────────
// OCR gives us lines like:
//   "From: To:"               ← trigger line
//   "John D. Miller  Innovate LLC"
//   "Address 123 ...  Attn: Sarah Chen, 789 ..."
//   "Email Address x@x.co  Email Address y@y.net"
//   "Phone number +1...  Phone number +1..."
void InvoiceParser::extractFromBlock(const QStringList &lines, InvoiceData &data) {
    // Find the "From:" line index
    int fromIdx = -1;
    for (int i = 0; i < lines.size(); i++) {
        if (lines[i].contains(QRegularExpression(R"(^from\s*:)", QRegularExpression::CaseInsensitiveOption))) {
            fromIdx = i;
            break;
        }
    }
    if (fromIdx < 0) return;

    // Scan next 5 lines for FROM data (left side of each line)
    for (int i = fromIdx + 1; i < qMin(fromIdx + 6, lines.size()); i++) {
        QString line = lines[i];

        // Name line — no keywords, just a name (before any "To:" content)
        if (data.fromName.isEmpty()) {
            // Split on 2+ spaces to separate FROM and TO columns
            QStringList cols = line.split(QRegularExpression(R"(\s{3,})"));
            if (!cols.isEmpty()) {
                QString left = cols[0].trimmed();
                // Must look like a name (no $ or digits at start)
                if (!left.isEmpty() && !left.contains('$')
                    && !left.contains(QRegularExpression(R"(^\d)"))) {
                    data.fromName = left;
                    if (cols.size() > 1)
                        data.toName = cols[1].trimmed();
                }
            }
            continue;
        }

        // Address line
        if (line.contains(QRegularExpression(R"(address\s)", QRegularExpression::CaseInsensitiveOption))
            && data.fromAddress.isEmpty()) {
            QStringList cols = line.split(QRegularExpression(R"(\s{3,})"));
            if (!cols.isEmpty()) {
                data.fromAddress = extractAddress(cols[0]);
                if (cols.size() > 1)
                    data.toAddress = extractAddress(cols[1]);
            }
            continue;
        }

        // Email line
        if (line.contains(QRegularExpression(R"(email)", QRegularExpression::CaseInsensitiveOption))
            && data.fromEmail.isEmpty()) {
            QStringList cols = line.split(QRegularExpression(R"(\s{3,})"));
            if (!cols.isEmpty()) {
                data.fromEmail = extractEmail(cols[0]);
                if (cols.size() > 1)
                    data.toEmail = extractEmail(cols[1]);
            }
            continue;
        }

        // Phone line
        if (line.contains(QRegularExpression(R"(phone)", QRegularExpression::CaseInsensitiveOption))
            && data.fromPhone.isEmpty()) {
            QStringList cols = line.split(QRegularExpression(R"(\s{3,})"));
            if (!cols.isEmpty()) {
                data.fromPhone = extractPhone(cols[0]);
                if (cols.size() > 1)
                    data.toPhone = extractPhone(cols[1]);
            }
            continue;
        }
    }
}

// ── TO Block ─────────────────────────────────────────────────────────────────
// Already extracted inside extractFromBlock above (right column)
// This function fills any gaps
void InvoiceParser::extractToBlock(const QStringList &lines, InvoiceData &data) {
    // If toName already found in extractFromBlock, skip
    if (!data.toName.isEmpty()) return;

    for (int i = 0; i < lines.size(); i++) {
        if (lines[i].contains(QRegularExpression(R"(\bto\s*:)", QRegularExpression::CaseInsensitiveOption))) {
            for (int j = i + 1; j < qMin(i + 6, lines.size()); j++) {
                QString line = lines[j];
                if (data.toName.isEmpty() && !line.contains('$')) {
                    data.toName = line.split(QRegularExpression(R"(\s{3,})"))[0].trimmed();
                }
                if (data.toEmail.isEmpty())
                    data.toEmail = extractEmail(line);
                if (data.toPhone.isEmpty())
                    data.toPhone = extractPhone(line);
                if (data.toAddress.isEmpty())
                    data.toAddress = extractAddress(line);
            }
            break;
        }
    }
}

// ── Line Items ───────────────────────────────────────────────────────────────
// OCR lines look like:
//   "1 Do It Yourself Tornado Kit a $ 489.00 $ 489.00"
//   "2 Earth Quake Pills 2 $ 44.00 $ 88.00"
//   "3 Electronic Motor 10 $ 50.00 20% $ 400.00"
QList<InvoiceItem> InvoiceParser::extractItems(const QStringList &lines) {
    QList<InvoiceItem> items;

    // Line item pattern:
    // ^(number) (item name text) (qty number) $ (unit cost) [discount%] $ (total)
    QRegularExpression itemRe(
        R"(^(\d+)\s+([A-Za-z][A-Za-z0-9 \-,\.\'&]+?)\s+(\d+(?:\.\d+)?)\s+\$?\s*([\d,]+\.?\d*)\s+(?:([\d.]+\s*%)\s+)?\$?\s*([\d,]+\.?\d*)$)"
        );

    // Also handle OCR misread qty (like 'a' instead of '1')
    QRegularExpression itemReFuzzy(
        R"(^(\d+)\s+([A-Za-z][A-Za-z0-9 \-,\.\'&]+?)\s+([a-zA-Z0-9]+)\s+\$?\s*([\d,]+\.?\d*)\s+(?:([\d.]+\s*%)\s+)?\$?\s*([\d,]+\.?\d*)$)"
        );

    for (const QString &line : lines) {
        // Skip header lines
        if (line.contains(QRegularExpression(R"(item\s*name|description|unit\s*cost|discount)",
                                             QRegularExpression::CaseInsensitiveOption)))
            continue;

        // Try strict match first
        auto m = itemRe.match(line);
        if (!m.hasMatch())
            m = itemReFuzzy.match(line);

        if (m.hasMatch()) {
            InvoiceItem item;
            item.itemId   = m.captured(1).toInt();
            item.itemName = m.captured(2).trimmed();

            // Handle OCR misread qty ('a' → 1, 'l' → 1, 'O' → 0)
            QString qtyStr = m.captured(3).trimmed().toLower();
            if (qtyStr == "a" || qtyStr == "l" || qtyStr == "i")
                item.qty = 1.0;
            else
                item.qty = qtyStr.toDouble();

            item.unitCost = parseAmount(m.captured(4));

            // Discount (optional)
            QString discStr = m.captured(5).trimmed();
            item.discount = discStr.isEmpty() ? 0.0 : parsePercent(discStr);

            item.total = parseAmount(m.captured(6));

            if (!item.itemName.isEmpty()) {
                items.append(item);
                qDebug() << "[Parser] Item found:"
                         << item.itemId << item.itemName
                         << item.qty << item.unitCost
                         << item.discount << item.total;
            }
        }
    }
    return items;
}

// ── Totals ───────────────────────────────────────────────────────────────────
void InvoiceParser::extractTotals(const QStringList &lines, InvoiceData &data) {
    // Subtotal — line with just "$ 977.00" or "977.00" after items
    QRegularExpression subtotalRe(R"(^\$?\s*([\d,]+\.\d{2})$)");

    // Tax rate — "Tax rate 10% $ 97.70"
    QRegularExpression taxRe(
        R"(tax\s*rate\s+([\d.]+)\s*%\s+\$?\s*([\d,]+\.?\d*))",
        QRegularExpression::CaseInsensitiveOption);

    // Total — "Total 13 $ 1,074.70"
    QRegularExpression totalRe(
        R"(total\s+\d*\s+\$?\s*([\d,]+\.?\d*))",
        QRegularExpression::CaseInsensitiveOption);

    for (const QString &line : lines) {
        // Subtotal
        if (data.subtotal == 0.0) {
            auto m = subtotalRe.match(line);
            if (m.hasMatch()) {
                double val = parseAmount(m.captured(1));
                // Subtotal is usually larger than any single item
                if (val > 100.0)
                    data.subtotal = val;
            }
        }

        // Tax
        {
            auto m = taxRe.match(line);
            if (m.hasMatch()) {
                data.taxRate   = m.captured(1).toDouble();
                data.taxAmount = parseAmount(m.captured(2));
            }
        }

        // Total
        {
            auto m = totalRe.match(line);
            if (m.hasMatch()) {
                double val = parseAmount(m.captured(1));
                if (val > data.subtotal)
                    data.totalAmount = val;
            }
        }
    }

    // Recalculate subtotal from items if not found
    if (data.subtotal == 0.0 && !data.items.isEmpty()) {
        double sum = 0;
        for (const InvoiceItem &it : data.items)
            sum += it.total;
        data.subtotal = sum;
    }

    // Recalculate tax amount if not found
    if (data.taxAmount == 0.0 && data.taxRate > 0)
        data.taxAmount = data.subtotal * data.taxRate / 100.0;

    // Recalculate total if not found
    if (data.totalAmount == 0.0)
        data.totalAmount = data.subtotal + data.taxAmount;
}

// ── Payment Note ─────────────────────────────────────────────────────────────
QString InvoiceParser::extractPaymentNote(const QStringList &lines) {
    QRegularExpression payRe(R"(please\s+make|payment\s+due|wire\s+transfer|bank\s+of)",
                             QRegularExpression::CaseInsensitiveOption);
    QStringList noteLines;
    bool capturing = false;

    for (const QString &line : lines) {
        if (payRe.match(line).hasMatch()) {
            capturing = true;
        }
        if (capturing) {
            if (line.contains(QRegularExpression(R"(signature)",
                                                 QRegularExpression::CaseInsensitiveOption)))
                break;
            noteLines << line;
        }
    }
    return noteLines.join(" ").trimmed();
}

// ── Helpers ──────────────────────────────────────────────────────────────────
double InvoiceParser::parseAmount(const QString &text) {
    QString c = text; c.remove(',').remove('$').remove('%').remove(' ');
    return c.toDouble();
}

double InvoiceParser::parsePercent(const QString &text) {
    QString c = text; c.remove('%').remove(' ');
    return c.toDouble();
}

QString InvoiceParser::extractEmail(const QString &text) {
    QRegularExpression re(R"([\w.\-+]+@[\w.\-]+\.[a-zA-Z]{2,})");
    auto m = re.match(text);
    return m.hasMatch() ? m.captured(0).trimmed() : "";
}

QString InvoiceParser::extractPhone(const QString &text) {
    QRegularExpression re(R"(\+?[\d\s\(\)\-]{10,20})");
    auto m = re.match(text);
    return m.hasMatch() ? m.captured(0).trimmed() : "";
}

QString InvoiceParser::extractAddress(const QString &text) {
    // Remove keyword prefixes like "Address", "Attn:"
    QString clean = text;
    clean.remove(QRegularExpression(R"(^(address|attn)\s*[:\-]?\s*)",
                                    QRegularExpression::CaseInsensitiveOption));
    // Remove email/phone if accidentally included
    clean.remove(QRegularExpression(R"([\w.\-+]+@[\w.\-]+)"));
    clean.remove(QRegularExpression(R"(\+?[\d\s\(\)\-]{10,20})"));
    return clean.trimmed();
}

bool InvoiceParser::isItemLine(const QString &line) {
    return line.contains(QRegularExpression(R"(^\d+\s+[A-Za-z])"));
}

bool InvoiceParser::lineContainsAmount(const QString &line) {
    return line.contains(QRegularExpression(R"(\$\s*[\d,]+\.\d{2})"));
}
