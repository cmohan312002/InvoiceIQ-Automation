# 🚀 InvoiceIQ - Intelligent Invoice OCR & Data Extraction System

InvoiceIQ is a smart OCR-based system designed to automatically extract, process, and structure data from invoices with high accuracy. It combines image processing, text recognition, and intelligent parsing to convert unstructured invoice data into usable structured formats.

---

## 🧠 Project Overview

Manual invoice processing is time-consuming and error-prone. InvoiceIQ solves this by:

* Detecting invoice regions (headers, fields, boxes)
* Extracting text using OCR
* Cleaning and correcting OCR noise
* Structuring key information like:

  * Invoice Number
  * Date
  * Sender & Receiver Details
  * Line Items
  * Total Amount
* Storing extracted data into a database for future use

---

## ⚙️ Key Features

* 📄 **Automatic Invoice Text Extraction**
* 📦 **Box/Field Detection for Structured Parsing**
* 🧹 **OCR Noise Cleaning & Correction**
* 🔍 **Dynamic Field Identification (No Hardcoding)**
* 💾 **SQLite Database Integration**
* ⚡ **Fast Processing Pipeline**
* 🧩 **Modular & Extendable Architecture**

---

## 🏗️ Tech Stack

* **Language:** C++
* **Framework:** Qt (for UI & processing)
* **OCR Engine:** Tesseract OCR
* **Image Processing:** OpenCV
* **Database:** SQLite (for structured data storage)
* **Data Handling:** Custom parsing logic

---

## 🔄 Workflow

1. Input invoice image/PDF
2. Preprocess image (resize, threshold, clean)
3. Detect regions/boxes
4. Apply OCR to extract raw text
5. Post-process text (fix OCR errors)
6. Parse and map data into structured fields
7. Store extracted data into SQLite database
8. Display or export structured data

---

## 💾 Database Design (SQLite)

Extracted invoice data is stored in a lightweight SQLite database for efficient retrieval and management.

### Tables:

**1. invoices**

* id (Primary Key)
* invoice_number
* date
* sender
* receiver
* total_amount

**2. line_items**

* id (Primary Key)
* invoice_id (Foreign Key)
* item_name
* quantity
* price
* total

### Benefits:

* Persistent storage
* Easy querying and filtering
* Lightweight and portable
* Suitable for desktop applications

---

## 📂 Project Structure

```plaintext
InvoiceIQ/
│
├── src/
│   ├── invoiceparser.cpp
│   ├── imageprocessor.cpp
│   ├── ocrengine.cpp
│   ├── database.cpp
│
├── include/
│   ├── invoiceparser.h
│   ├── database.h
│   ├── utils.h
│
├── assets/
│   ├── sample_invoices/
│
├── output/
│   ├── extracted_data/
│
└── README.md
```

---

## 🚀 How to Run

### Prerequisites

* Install Tesseract OCR
* Install OpenCV
* Install SQLite
* Qt Creator (recommended)

### Steps

```bash
# Clone the repository
git clone https://github.com/your-username/InvoiceIQ.git

# Open the project in Qt Creator
# Build and run the application
```

---

## 🎯 Use Cases

* Accounts & Finance Automation
* Invoice Digitization Systems
* ERP Integration
* Document Processing Pipelines
* AI-based Document Intelligence Systems
* 📊 Data Storage & Retrieval using SQLite
* 📁 Historical Invoice Management

---

## 💡 What Makes This Project Special?

Unlike basic OCR tools, InvoiceIQ:

* Focuses on **structured data extraction**
* Uses **dynamic parsing instead of hardcoded fields**
* Combines **computer vision + logic**, not just text recognition
* Stores extracted data for **real-world usability**

---

## 🧪 Future Enhancements

* 🔥 Deep Learning-based field detection
* 🌐 Web-based interface (React / Angular)
* ☁️ Cloud deployment (Azure / AWS)
* 📊 Analytics dashboard
* 🧾 Multi-format document support

---

## 👨‍💻 Author

**Mohan Chilivery**
Software Engineer | MCA Student | Full Stack Developer

---

## ⭐ Support

If you found this project useful:

* Star ⭐ the repo
* Share it with others
* Contribute to improve it

---

## 📜 License

This project is open-source and available under the MIT License.
