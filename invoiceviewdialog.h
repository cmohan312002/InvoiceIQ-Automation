#pragma once
#include <QDialog>
#include "invoicedata.h"

class QLineEdit;
class QDateEdit;
class QTableWidget;
class QTextEdit;
class QDoubleSpinBox;
class QTabWidget;
class QPushButton;

class InvoiceViewDialog : public QDialog {
    Q_OBJECT
public:
    explicit InvoiceViewDialog(const InvoiceData &data, bool editable = false, QWidget *parent = nullptr);
    InvoiceData getEditedData() const;

private slots:
    void addItemRow();
    void removeItemRow();
    void recalcTotals();

private:
    void buildUI();
    void populateFields();
    void setupItemsTable();

    InvoiceData m_data;
    bool        m_editable;

    // Header
    QLineEdit   *m_invNumber;
    QDateEdit   *m_invDate;

    // From
    QLineEdit   *m_fromName, *m_fromAddr, *m_fromEmail, *m_fromPhone;

    // To
    QLineEdit   *m_toName, *m_toAddr, *m_toEmail, *m_toPhone;

    // Items
    QTableWidget *m_itemsTable;

    // Totals
    QDoubleSpinBox *m_subtotal, *m_taxRate, *m_taxAmount, *m_totalAmount;

    // Note / raw
    QTextEdit   *m_paymentNote;
    QTextEdit   *m_rawText;

    QPushButton *m_addRowBtn, *m_removeRowBtn;
    QTabWidget  *m_tabs;
};
