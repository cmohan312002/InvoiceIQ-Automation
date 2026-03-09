#pragma once
#include <QString>
#include <QList>
#include <QDate>

struct InvoiceItem {
    int     itemId    = 0;
    QString itemName;
    double  qty       = 0.0;
    double  unitCost  = 0.0;
    double  discount  = 0.0;
    double  total     = 0.0;
};

struct InvoiceData {
    int     dbId          = -1;
    QString invoiceNumber;
    QDate   invoiceDate;

    QString fromName;
    QString fromAddress;
    QString fromEmail;
    QString fromPhone;

    QString toName;
    QString toAddress;
    QString toEmail;
    QString toPhone;

    QList<InvoiceItem> items;

    double subtotal    = 0.0;
    double taxRate     = 0.0;
    double taxAmount   = 0.0;
    double totalAmount = 0.0;

    QString paymentNote;
    QString rawText;
};
