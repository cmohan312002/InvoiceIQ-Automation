#include "databasemanager.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QVariant>
#include <QDebug>

DatabaseManager::DatabaseManager() {}
DatabaseManager::~DatabaseManager() { close(); }

bool DatabaseManager::open(const QString &dbPath) {
    m_db = QSqlDatabase::addDatabase("QSQLITE", "invoice_conn");
    m_db.setDatabaseName(dbPath);
    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        return false;
    }
    return createTables();
}

void DatabaseManager::close() {
    if (m_db.isOpen()) m_db.close();
}

bool DatabaseManager::isOpen() const { return m_db.isOpen(); }

bool DatabaseManager::createTables() {
    QSqlQuery q(m_db);

    bool ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS invoices (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            invoice_number  TEXT,
            invoice_date    TEXT,
            from_name       TEXT,
            from_address    TEXT,
            from_email      TEXT,
            from_phone      TEXT,
            to_name         TEXT,
            to_address      TEXT,
            to_email        TEXT,
            to_phone        TEXT,
            subtotal        REAL DEFAULT 0,
            tax_rate        REAL DEFAULT 0,
            tax_amount      REAL DEFAULT 0,
            total_amount    REAL DEFAULT 0,
            payment_note    TEXT,
            raw_text        TEXT,
            created_at      TEXT DEFAULT (datetime('now'))
        )
    )");
    if (!ok) { m_lastError = q.lastError().text(); return false; }

    ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS invoice_items (
            id           INTEGER PRIMARY KEY AUTOINCREMENT,
            invoice_id   INTEGER NOT NULL,
            item_order   INTEGER DEFAULT 0,
            item_name    TEXT,
            qty          REAL DEFAULT 0,
            unit_cost    REAL DEFAULT 0,
            discount     REAL DEFAULT 0,
            total        REAL DEFAULT 0,
            FOREIGN KEY (invoice_id) REFERENCES invoices(id) ON DELETE CASCADE
        )
    )");
    if (!ok) { m_lastError = q.lastError().text(); return false; }
    return true;
}

bool DatabaseManager::saveInvoice(InvoiceData &data) {
    QSqlQuery q(m_db);
    q.prepare(R"(
        INSERT INTO invoices
        (invoice_number, invoice_date, from_name, from_address, from_email, from_phone,
         to_name, to_address, to_email, to_phone,
         subtotal, tax_rate, tax_amount, total_amount, payment_note, raw_text)
        VALUES
        (:inv_num, :inv_date, :from_name, :from_addr, :from_email, :from_phone,
         :to_name, :to_addr, :to_email, :to_phone,
         :subtotal, :tax_rate, :tax_amount, :total_amount, :pay_note, :raw_text)
    )");
    q.bindValue(":inv_num",      data.invoiceNumber);
    q.bindValue(":inv_date",     data.invoiceDate.toString("yyyy-MM-dd"));
    q.bindValue(":from_name",    data.fromName);
    q.bindValue(":from_addr",    data.fromAddress);
    q.bindValue(":from_email",   data.fromEmail);
    q.bindValue(":from_phone",   data.fromPhone);
    q.bindValue(":to_name",      data.toName);
    q.bindValue(":to_addr",      data.toAddress);
    q.bindValue(":to_email",     data.toEmail);
    q.bindValue(":to_phone",     data.toPhone);
    q.bindValue(":subtotal",     data.subtotal);
    q.bindValue(":tax_rate",     data.taxRate);
    q.bindValue(":tax_amount",   data.taxAmount);
    q.bindValue(":total_amount", data.totalAmount);
    q.bindValue(":pay_note",     data.paymentNote);
    q.bindValue(":raw_text",     data.rawText);

    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    data.dbId = q.lastInsertId().toInt();
    return saveItems(data.dbId, data.items);
}

bool DatabaseManager::saveItems(int invoiceId, const QList<InvoiceItem> &items) {
    QSqlQuery q(m_db);
    q.prepare(R"(
        INSERT INTO invoice_items (invoice_id, item_order, item_name, qty, unit_cost, discount, total)
        VALUES (:inv_id, :order, :name, :qty, :unit_cost, :discount, :total)
    )");
    for (int i = 0; i < items.size(); ++i) {
        const InvoiceItem &item = items[i];
        q.bindValue(":inv_id",    invoiceId);
        q.bindValue(":order",     i + 1);
        q.bindValue(":name",      item.itemName);
        q.bindValue(":qty",       item.qty);
        q.bindValue(":unit_cost", item.unitCost);
        q.bindValue(":discount",  item.discount);
        q.bindValue(":total",     item.total);
        if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    }
    return true;
}

bool DatabaseManager::updateInvoice(const InvoiceData &data) {
    QSqlQuery q(m_db);
    q.prepare(R"(
        UPDATE invoices SET
            invoice_number=:inv_num, invoice_date=:inv_date,
            from_name=:from_name, from_address=:from_addr,
            from_email=:from_email, from_phone=:from_phone,
            to_name=:to_name, to_address=:to_addr,
            to_email=:to_email, to_phone=:to_phone,
            subtotal=:subtotal, tax_rate=:tax_rate,
            tax_amount=:tax_amount, total_amount=:total_amount,
            payment_note=:pay_note, raw_text=:raw_text
        WHERE id=:id
    )");
    q.bindValue(":id",           data.dbId);
    q.bindValue(":inv_num",      data.invoiceNumber);
    q.bindValue(":inv_date",     data.invoiceDate.toString("yyyy-MM-dd"));
    q.bindValue(":from_name",    data.fromName);
    q.bindValue(":from_addr",    data.fromAddress);
    q.bindValue(":from_email",   data.fromEmail);
    q.bindValue(":from_phone",   data.fromPhone);
    q.bindValue(":to_name",      data.toName);
    q.bindValue(":to_addr",      data.toAddress);
    q.bindValue(":to_email",     data.toEmail);
    q.bindValue(":to_phone",     data.toPhone);
    q.bindValue(":subtotal",     data.subtotal);
    q.bindValue(":tax_rate",     data.taxRate);
    q.bindValue(":tax_amount",   data.taxAmount);
    q.bindValue(":total_amount", data.totalAmount);
    q.bindValue(":pay_note",     data.paymentNote);
    q.bindValue(":raw_text",     data.rawText);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }

    // Delete & re-insert items
    QSqlQuery dq(m_db);
    dq.prepare("DELETE FROM invoice_items WHERE invoice_id=:id");
    dq.bindValue(":id", data.dbId);
    dq.exec();
    return saveItems(data.dbId, data.items);
}

bool DatabaseManager::deleteInvoice(int dbId) {
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM invoices WHERE id=:id");
    q.bindValue(":id", dbId);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return true;
}

InvoiceData DatabaseManager::loadInvoice(int dbId) {
    InvoiceData data;
    QSqlQuery q(m_db);
    q.prepare("SELECT * FROM invoices WHERE id=:id");
    q.bindValue(":id", dbId);
    if (!q.exec() || !q.next()) return data;

    data.dbId          = q.value("id").toInt();
    data.invoiceNumber = q.value("invoice_number").toString();
    data.invoiceDate   = QDate::fromString(q.value("invoice_date").toString(), "yyyy-MM-dd");
    data.fromName      = q.value("from_name").toString();
    data.fromAddress   = q.value("from_address").toString();
    data.fromEmail     = q.value("from_email").toString();
    data.fromPhone     = q.value("from_phone").toString();
    data.toName        = q.value("to_name").toString();
    data.toAddress     = q.value("to_address").toString();
    data.toEmail       = q.value("to_email").toString();
    data.toPhone       = q.value("to_phone").toString();
    data.subtotal      = q.value("subtotal").toDouble();
    data.taxRate       = q.value("tax_rate").toDouble();
    data.taxAmount     = q.value("tax_amount").toDouble();
    data.totalAmount   = q.value("total_amount").toDouble();
    data.paymentNote   = q.value("payment_note").toString();
    data.rawText       = q.value("raw_text").toString();
    data.items         = loadItems(dbId);
    return data;
}

QList<InvoiceItem> DatabaseManager::loadItems(int invoiceId) {
    QList<InvoiceItem> items;
    QSqlQuery q(m_db);
    q.prepare("SELECT * FROM invoice_items WHERE invoice_id=:id ORDER BY item_order");
    q.bindValue(":id", invoiceId);
    if (!q.exec()) return items;
    while (q.next()) {
        InvoiceItem item;
        item.itemId   = q.value("item_order").toInt();
        item.itemName = q.value("item_name").toString();
        item.qty      = q.value("qty").toDouble();
        item.unitCost = q.value("unit_cost").toDouble();
        item.discount = q.value("discount").toDouble();
        item.total    = q.value("total").toDouble();
        items.append(item);
    }
    return items;
}

QList<InvoiceData> DatabaseManager::loadAllInvoices() {
    QList<InvoiceData> list;
    QSqlQuery q(m_db);
    q.exec("SELECT id, invoice_number, invoice_date, from_name, to_name, total_amount FROM invoices ORDER BY id DESC");
    while (q.next()) {
        InvoiceData d;
        d.dbId          = q.value("id").toInt();
        d.invoiceNumber = q.value("invoice_number").toString();
        d.invoiceDate   = QDate::fromString(q.value("invoice_date").toString(), "yyyy-MM-dd");
        d.fromName      = q.value("from_name").toString();
        d.toName        = q.value("to_name").toString();
        d.totalAmount   = q.value("total_amount").toDouble();
        list.append(d);
    }
    return list;
}

QList<InvoiceData> DatabaseManager::searchInvoices(const QString &keyword) {
    QList<InvoiceData> list;
    QSqlQuery q(m_db);
    q.prepare(R"(
        SELECT id, invoice_number, invoice_date, from_name, to_name, total_amount
        FROM invoices
        WHERE invoice_number LIKE :kw OR from_name LIKE :kw OR to_name LIKE :kw
        ORDER BY id DESC
    )");
    q.bindValue(":kw", "%" + keyword + "%");
    q.exec();
    while (q.next()) {
        InvoiceData d;
        d.dbId          = q.value("id").toInt();
        d.invoiceNumber = q.value("invoice_number").toString();
        d.invoiceDate   = QDate::fromString(q.value("invoice_date").toString(), "yyyy-MM-dd");
        d.fromName      = q.value("from_name").toString();
        d.toName        = q.value("to_name").toString();
        d.totalAmount   = q.value("total_amount").toDouble();
        list.append(d);
    }
    return list;
}
