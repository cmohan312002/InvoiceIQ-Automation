#pragma once
#include "invoicedata.h"
#include <QString>
#include <QList>
#include <QtSql/QSqlDatabase>

class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();

    bool    open(const QString &dbPath = "invoices.db");
    void    close();
    bool    isOpen() const;

    bool    saveInvoice(InvoiceData &data);        // sets data.dbId on success
    bool    updateInvoice(const InvoiceData &data);
    bool    deleteInvoice(int dbId);
    InvoiceData loadInvoice(int dbId);
    QList<InvoiceData> loadAllInvoices();          // summary list
    QList<InvoiceData> searchInvoices(const QString &keyword);

    QString lastError() const { return m_lastError; }

private:
    QSqlDatabase m_db;
    QString      m_lastError;

    bool createTables();
    bool saveItems(int invoiceId, const QList<InvoiceItem> &items);
    QList<InvoiceItem> loadItems(int invoiceId);
};
