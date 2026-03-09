#include "invoiceviewdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QDateEdit>
#include <QTableWidget>
#include <QTextEdit>
#include <QDoubleSpinBox>
#include <QTabWidget>
#include <QPushButton>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QLabel>
#include <QScrollArea>

InvoiceViewDialog::InvoiceViewDialog(const InvoiceData &data, bool editable, QWidget *parent)
    : QDialog(parent), m_data(data), m_editable(editable)
{
    setWindowTitle(editable ? "Edit Invoice" : "Invoice Details");
    setMinimumSize(850, 680);
    setStyleSheet(R"(
        QDialog, QWidget {
            background: #1a1a2e;
            color: #e0e0e0;
            font-family: Segoe UI;
            font-size: 13px;
        }
        QGroupBox {
            border: 1px solid #3a3a5c;
            border-radius: 6px;
            margin-top: 8px;
            padding: 8px;
            font-weight: bold;
            color: #a0c4ff;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
        }
        QLineEdit {
            background: #252545;
            border: 1px solid #4a4a7a;
            border-radius: 4px;
            padding: 4px 8px;
            color: #e0e0e0;
        }
        QLineEdit:focus {
            border-color: #7a7aff;
        }
        QLineEdit:read-only {
            background: #1e1e3a;
            border-color: #2a2a4a;
            color: #a0a0c0;
        }
        QDateEdit {
            background: #252545;
            border: 1px solid #4a4a7a;
            border-radius: 4px;
            padding: 4px 8px;
            color: #e0e0e0;
        }
        QDateEdit:read-only {
            background: #1e1e3a;
            border-color: #2a2a4a;
            color: #a0a0c0;
        }
        QDateEdit::drop-down {
            border: none;
            background: #3a3a8a;
            border-radius: 3px;
        }
        QDoubleSpinBox {
            background: #252545;
            border: 1px solid #4a4a7a;
            border-radius: 4px;
            padding: 4px;
            color: #e0e0e0;
        }
        QDoubleSpinBox:read-only {
            background: #1e1e3a;
            border-color: #2a2a4a;
            color: #a0a0c0;
        }
        QTextEdit {
            background: #12122a;
            border: 1px solid #3a3a5c;
            border-radius: 4px;
            color: #c8c8d8;
        }
        QTableWidget {
            background: #12122a;
            gridline-color: #2a2a4a;
            selection-background-color: #4a4aaa;
            border: none;
            border-radius: 4px;
        }
        QHeaderView::section {
            background: #252550;
            color: #a0c4ff;
            padding: 6px;
            border: none;
            font-weight: bold;
        }
        QTableWidget::item:alternate {
            background: #16163a;
        }
        QPushButton {
            background: #3a3a8a;
            color: #ffffff;
            border: none;
            border-radius: 5px;
            padding: 6px 14px;
            font-weight: bold;
        }
        QPushButton:hover   { background: #5a5acc; }
        QPushButton:pressed { background: #2a2a6a; }
        QTabWidget::pane {
            border: 1px solid #3a3a5c;
            border-radius: 4px;
        }
        QTabBar::tab {
            background: #252545;
            color: #a0a0c0;
            padding: 7px 16px;
            border-radius: 4px 4px 0 0;
            margin-right: 2px;
        }
        QTabBar::tab:selected {
            background: #3a3a8a;
            color: #ffffff;
        }
        QScrollBar:vertical {
            background: #1a1a2e;
            width: 10px;
        }
        QScrollBar::handle:vertical {
            background: #3a3a6a;
            border-radius: 5px;
        }
        QLabel {
            color: #e0e0e0;
        }
        QDialogButtonBox QPushButton {
            background: #3a3a8a;
            color: #ffffff;
            border: none;
            border-radius: 5px;
            padding: 6px 20px;
            font-weight: bold;
            min-width: 80px;
        }
        QDialogButtonBox QPushButton:hover { background: #5a5acc; }
        QCalendarWidget {
            background: #1a1a2e;
            color: #e0e0e0;
        }
        QCalendarWidget QAbstractItemView {
            background: #1a1a2e;
            color: #e0e0e0;
            selection-background-color: #3a3a8a;
        }
    )");

    buildUI();
    populateFields();
}

void InvoiceViewDialog::buildUI() {
    m_tabs = new QTabWidget(this);

    // ── Tab 1: Details ──────────────────────────────────────────────
    QWidget *detailTab = new QWidget;
    QVBoxLayout *dv = new QVBoxLayout(detailTab);

    // Header group
    QGroupBox *hdrGrp = new QGroupBox("Invoice Header");
    QFormLayout *hdrForm = new QFormLayout(hdrGrp);
    m_invNumber = new QLineEdit; m_invNumber->setReadOnly(!m_editable);
    m_invDate   = new QDateEdit; m_invDate->setCalendarPopup(true); m_invDate->setReadOnly(!m_editable);
    hdrForm->addRow("Invoice #:", m_invNumber);
    hdrForm->addRow("Date:",      m_invDate);

    // From/To side by side
    QHBoxLayout *fromToLayout = new QHBoxLayout;

    QGroupBox *fromGrp = new QGroupBox("From");
    QFormLayout *fromForm = new QFormLayout(fromGrp);
    m_fromName  = new QLineEdit; m_fromName->setReadOnly(!m_editable);
    m_fromAddr  = new QLineEdit; m_fromAddr->setReadOnly(!m_editable);
    m_fromEmail = new QLineEdit; m_fromEmail->setReadOnly(!m_editable);
    m_fromPhone = new QLineEdit; m_fromPhone->setReadOnly(!m_editable);
    fromForm->addRow("Name:",    m_fromName);
    fromForm->addRow("Address:", m_fromAddr);
    fromForm->addRow("Email:",   m_fromEmail);
    fromForm->addRow("Phone:",   m_fromPhone);

    QGroupBox *toGrp = new QGroupBox("To");
    QFormLayout *toForm = new QFormLayout(toGrp);
    m_toName  = new QLineEdit; m_toName->setReadOnly(!m_editable);
    m_toAddr  = new QLineEdit; m_toAddr->setReadOnly(!m_editable);
    m_toEmail = new QLineEdit; m_toEmail->setReadOnly(!m_editable);
    m_toPhone = new QLineEdit; m_toPhone->setReadOnly(!m_editable);
    toForm->addRow("Name:",    m_toName);
    toForm->addRow("Address:", m_toAddr);
    toForm->addRow("Email:",   m_toEmail);
    toForm->addRow("Phone:",   m_toPhone);

    fromToLayout->addWidget(fromGrp);
    fromToLayout->addWidget(toGrp);

    dv->addWidget(hdrGrp);
    dv->addLayout(fromToLayout);

    // ── Tab 2: Items ────────────────────────────────────────────────
    QWidget *itemTab = new QWidget;
    QVBoxLayout *iv = new QVBoxLayout(itemTab);

    setupItemsTable();

    if (m_editable) {
        QHBoxLayout *btnRow = new QHBoxLayout;
        m_addRowBtn    = new QPushButton("+ Add Item");
        m_removeRowBtn = new QPushButton("- Remove Selected");
        btnRow->addWidget(m_addRowBtn);
        btnRow->addWidget(m_removeRowBtn);
        btnRow->addStretch();
        iv->addLayout(btnRow);
        connect(m_addRowBtn,    &QPushButton::clicked, this, &InvoiceViewDialog::addItemRow);
        connect(m_removeRowBtn, &QPushButton::clicked, this, &InvoiceViewDialog::removeItemRow);
    }
    iv->addWidget(m_itemsTable);

    // Totals
    QGroupBox *totGrp = new QGroupBox("Totals");
    QFormLayout *totForm = new QFormLayout(totGrp);

    auto makeSpin = [&](bool ro) {
        auto *s = new QDoubleSpinBox;
        s->setMaximum(9999999); s->setPrefix("$ "); s->setDecimals(2);
        s->setReadOnly(ro); return s;
    };
    m_subtotal    = makeSpin(!m_editable);
    m_taxRate     = new QDoubleSpinBox; m_taxRate->setSuffix(" %"); m_taxRate->setMaximum(100); m_taxRate->setReadOnly(!m_editable);
    m_taxAmount   = makeSpin(!m_editable);
    m_totalAmount = makeSpin(!m_editable);

    totForm->addRow("Subtotal:",    m_subtotal);
    totForm->addRow("Tax Rate:",    m_taxRate);
    totForm->addRow("Tax Amount:",  m_taxAmount);
    totForm->addRow("Total:",       m_totalAmount);

    if (m_editable)
        connect(m_taxRate, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &InvoiceViewDialog::recalcTotals);

    iv->addWidget(totGrp);

    // ── Tab 3: Notes ────────────────────────────────────────────────
    QWidget *noteTab = new QWidget;
    QVBoxLayout *nv = new QVBoxLayout(noteTab);
    m_paymentNote = new QTextEdit; m_paymentNote->setReadOnly(!m_editable);
    m_paymentNote->setPlaceholderText("Payment instructions...");
    nv->addWidget(new QLabel("Payment Note:"));
    nv->addWidget(m_paymentNote);

    // ── Tab 4: Raw OCR ──────────────────────────────────────────────
    QWidget *rawTab = new QWidget;
    QVBoxLayout *rv = new QVBoxLayout(rawTab);
    m_rawText = new QTextEdit; m_rawText->setReadOnly(true);
    m_rawText->setFont(QFont("Courier New", 9));
    rv->addWidget(new QLabel("Raw OCR Output:"));
    rv->addWidget(m_rawText);

    m_tabs->addTab(detailTab, "Details");
    m_tabs->addTab(itemTab,   "Line Items");
    m_tabs->addTab(noteTab,   "Notes");
    m_tabs->addTab(rawTab,    "Raw OCR");

    QDialogButtonBox *bbox = new QDialogButtonBox(
        m_editable ? (QDialogButtonBox::Save | QDialogButtonBox::Cancel)
                   :  QDialogButtonBox::Close);
    connect(bbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *main = new QVBoxLayout(this);
    main->addWidget(m_tabs);
    main->addWidget(bbox);
}

void InvoiceViewDialog::setupItemsTable() {
    m_itemsTable = new QTableWidget(0, 6);
    m_itemsTable->setHorizontalHeaderLabels({"#","Item Name","Qty","Unit Cost ($)","Discount (%)","Total ($)"});
    m_itemsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_itemsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_itemsTable->setEditTriggers(m_editable ? QAbstractItemView::DoubleClicked : QAbstractItemView::NoEditTriggers);
    m_itemsTable->setAlternatingRowColors(true);
}

void InvoiceViewDialog::populateFields() {
    m_invNumber->setText(m_data.invoiceNumber);
    m_invDate->setDate(m_data.invoiceDate.isValid() ? m_data.invoiceDate : QDate::currentDate());

    m_fromName->setText(m_data.fromName);
    m_fromAddr->setText(m_data.fromAddress);
    m_fromEmail->setText(m_data.fromEmail);
    m_fromPhone->setText(m_data.fromPhone);

    m_toName->setText(m_data.toName);
    m_toAddr->setText(m_data.toAddress);
    m_toEmail->setText(m_data.toEmail);
    m_toPhone->setText(m_data.toPhone);

    m_itemsTable->setRowCount(0);
    for (const InvoiceItem &item : m_data.items) {
        int row = m_itemsTable->rowCount();
        m_itemsTable->insertRow(row);
        m_itemsTable->setItem(row, 0, new QTableWidgetItem(QString::number(item.itemId)));
        m_itemsTable->setItem(row, 1, new QTableWidgetItem(item.itemName));
        m_itemsTable->setItem(row, 2, new QTableWidgetItem(QString::number(item.qty)));
        m_itemsTable->setItem(row, 3, new QTableWidgetItem(QString::number(item.unitCost, 'f', 2)));
        m_itemsTable->setItem(row, 4, new QTableWidgetItem(QString::number(item.discount, 'f', 1)));
        m_itemsTable->setItem(row, 5, new QTableWidgetItem(QString::number(item.total, 'f', 2)));
    }

    m_subtotal->setValue(m_data.subtotal);
    m_taxRate->setValue(m_data.taxRate);
    m_taxAmount->setValue(m_data.taxAmount);
    m_totalAmount->setValue(m_data.totalAmount);
    m_paymentNote->setPlainText(m_data.paymentNote);
    m_rawText->setPlainText(m_data.rawText);
}

void InvoiceViewDialog::addItemRow() {
    int row = m_itemsTable->rowCount();
    m_itemsTable->insertRow(row);
    m_itemsTable->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
    for (int c = 1; c < 6; ++c)
        m_itemsTable->setItem(row, c, new QTableWidgetItem("0"));
}

void InvoiceViewDialog::removeItemRow() {
    int row = m_itemsTable->currentRow();
    if (row >= 0) m_itemsTable->removeRow(row);
}

void InvoiceViewDialog::recalcTotals() {
    double sub = 0;
    for (int r = 0; r < m_itemsTable->rowCount(); ++r) {
        auto *it = m_itemsTable->item(r, 5);
        if (it) sub += it->text().toDouble();
    }
    m_subtotal->setValue(sub);
    double tax = sub * m_taxRate->value() / 100.0;
    m_taxAmount->setValue(tax);
    m_totalAmount->setValue(sub + tax);
}

InvoiceData InvoiceViewDialog::getEditedData() const {
    InvoiceData d = m_data;
    d.invoiceNumber = m_invNumber->text();
    d.invoiceDate   = m_invDate->date();
    d.fromName      = m_fromName->text();
    d.fromAddress   = m_fromAddr->text();
    d.fromEmail     = m_fromEmail->text();
    d.fromPhone     = m_fromPhone->text();
    d.toName        = m_toName->text();
    d.toAddress     = m_toAddr->text();
    d.toEmail       = m_toEmail->text();
    d.toPhone       = m_toPhone->text();
    d.subtotal      = m_subtotal->value();
    d.taxRate       = m_taxRate->value();
    d.taxAmount     = m_taxAmount->value();
    d.totalAmount   = m_totalAmount->value();
    d.paymentNote   = m_paymentNote->toPlainText();

    d.items.clear();
    for (int r = 0; r < m_itemsTable->rowCount(); ++r) {
        InvoiceItem item;
        item.itemId   = m_itemsTable->item(r,0) ? m_itemsTable->item(r,0)->text().toInt() : r+1;
        item.itemName = m_itemsTable->item(r,1) ? m_itemsTable->item(r,1)->text() : "";
        item.qty      = m_itemsTable->item(r,2) ? m_itemsTable->item(r,2)->text().toDouble() : 0;
        item.unitCost = m_itemsTable->item(r,3) ? m_itemsTable->item(r,3)->text().toDouble() : 0;
        item.discount = m_itemsTable->item(r,4) ? m_itemsTable->item(r,4)->text().toDouble() : 0;
        item.total    = m_itemsTable->item(r,5) ? m_itemsTable->item(r,5)->text().toDouble() : 0;
        if (!item.itemName.isEmpty()) d.items.append(item);
    }
    return d;
}
