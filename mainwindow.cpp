#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    //Исходное состояние виджетов
    ui->setupUi(this);
    ui->lb_statusConnect->setStyleSheet("color:red");
    ui->pb_request->setEnabled(false);

    /*
     * Выделим память под необходимые объекты. Все они наследники
     * QObject, поэтому воспользуемся иерархией.
    */

    mySettingsDialog = new Dialog(this);
    dataBase = new DataBase(this);
    msg = new QMessageBox(this);

    dataForConnect.resize(NUM_DATA_FOR_CONNECT_TO_DB);

    // Прописываем данные по умолчанию
    dataForConnect[hostName] = "981757-ca08998.tmweb.ru"; // Host
    dataForConnect[dbName]   = "netology_cpp";            // DB Name
    dataForConnect[login]    = "netology_usr_cpp";        // Login
    dataForConnect[pass]     = "CppNeto3";                // Password
    dataForConnect[port]     = "5432";

    //Установим размер вектора данных для подключения к БД
    dataForConnect.resize(NUM_DATA_FOR_CONNECT_TO_DB);

    /*
     * Добавим БД используя стандартный драйвер PSQL и зададим имя.
    */
    dataBase->AddDataBase(POSTGRE_DRIVER, DB_NAME);

    /*
     * Устанавливаем данные для подключениея к БД.
     * Поскольку метод небольшой используем лямбда-функцию.
     */


    /*
     *  Сигнал для подключения к БД
     */
    connect(dataBase, &DataBase::sig_SendStatusConnection, this, &MainWindow::ReceiveStatusConnectionToDB);
    connect(ui->pb_clear, &QPushButton::clicked, this, &MainWindow::on_pb_clear_clicked);

}

MainWindow::~MainWindow()
{
    delete ui;
}

/*!
 * @brief Слот отображает форму для ввода данных подключения к БД
 */
void MainWindow::on_act_addData_triggered()
{
    if (mySettingsDialog->exec() == QDialog::Accepted) {
           dataForConnect[hostName] = mySettingsDialog->getHostName();
           dataForConnect[dbName]   = mySettingsDialog->getDbName();
           dataForConnect[login]    = mySettingsDialog->getLogin();
           dataForConnect[pass]     = mySettingsDialog->getPassword();
           dataForConnect[port]     = QString::number(mySettingsDialog->getPort());

           qDebug() << "Данные для подключения обновлены!";
       }
}

/*!
 * @brief Слот выполняет подключение к БД. И отображает ошибки.
 */

void MainWindow::on_act_connect_triggered()
{
    /*
     * Обработчик кнопки у нас должен подключаться и отключаться от БД.
     * Можно привязаться к надписи лейбла статуса. Если он равен
     * "Отключено" мы осуществляем подключение, если "Подключено" то
     * отключаемся
    */

    if(ui->lb_statusConnect->text() == "Отключено"){

       ui->lb_statusConnect->setText("Подключение");
       ui->lb_statusConnect->setStyleSheet("color : black");


       auto conn = [&]{dataBase->ConnectToDataBase(dataForConnect);};
       QtConcurrent::run(conn);

    }
    else{
        dataBase->DisconnectFromDataBase(DB_NAME);
        ui->lb_statusConnect->setText("Отключено");
        ui->act_connect->setText("Подключиться");
        ui->lb_statusConnect->setStyleSheet("color:red");
        ui->pb_request->setEnabled(false);
    }

}

/*!
 * \brief Обработчик кнопки "Получить"
 */
void MainWindow::on_pb_request_clicked()
{
    qDebug() << "Выбранный фильтр:" << ui->cb_category->currentText();
    // Получаем выбранный фильтр (предположим, это QComboBox с именем cb_filter)
    QString filter = ui->cb_category->currentText();

    // Получаем соединение с БД по имени (DB_NAME из прекода)
    QSqlDatabase db = QSqlDatabase::database(DB_NAME);

    if (filter == "Все") {
        // --- ИСПОЛЬЗУЕМ QSqlTableModel ---
        if (tableModel) delete tableModel;
        tableModel = new QSqlTableModel(this, db);
        tableModel->setTable("film");
        tableModel->select();

         ui->tableView->setModel(tableModel);

        // Скрываем все столбцы, кроме title и description
        for (int i = 0; i < tableModel->columnCount(); ++i) {
            QString header = tableModel->headerData(i, Qt::Horizontal).toString();
            if (header != "title" && header != "description") {
                ui->tableView->setColumnHidden(i, true);
            }
        }
        // Устанавливаем заголовки (индексы могут отличаться, проверьте по вашей БД)
        tableModel->setHeaderData(1, Qt::Horizontal, "Название фильма");
        tableModel->setHeaderData(2, Qt::Horizontal, "Описание фильма");


    }
    else if (filter == "Комедии" || filter == "Ужасы") {
        // --- ИСПОЛЬЗУЕМ QSqlQueryModel ---
        if (queryModel) delete queryModel;
        queryModel = new QSqlQueryModel(this);

        QString category = (filter == "Комедии") ? "Comedy" : "Horror";
        QString queryStr = QString(
            "SELECT f.title, f.description FROM film f "
            "JOIN film_category fc ON f.film_id = fc.film_id "
            "JOIN category c ON c.category_id = fc.category_id "
            "WHERE c.name = '%1'").arg(category);

        queryModel->setQuery(queryStr, db);

        if (queryModel->lastError().isValid()) {
                    qDebug() << "Ошибка запроса:" << queryModel->lastError().text();
                } else {
                    qDebug() << "Запрос выполнен. Строк получено:" << queryModel->rowCount();
                }

        // Заголовки для QSqlQueryModel (индексы 0 и 1)
        queryModel->setHeaderData(0, Qt::Horizontal, "Название фильма");
        queryModel->setHeaderData(1, Qt::Horizontal, "Описание фильма");

        ui->tableView->setModel(queryModel);
    }
}

/*!
 * \brief Слот отображает значение в QTableWidget
 * \param widget
 * \param typeRequest
 */



/*!
 * \brief Метод изменяет стотояние формы в зависимости от статуса подключения к БД
 * \param status
 */
void MainWindow::ReceiveStatusConnectionToDB(bool status)
{
    if(status){
        ui->act_connect->setText("Отключиться");
        ui->lb_statusConnect->setText("Подключено к БД");
        ui->lb_statusConnect->setStyleSheet("color:green");
        ui->pb_request->setEnabled(true);
    }
    else{
        dataBase->DisconnectFromDataBase(DB_NAME);
        msg->setIcon(QMessageBox::Critical);
        msg->setText(dataBase->GetLastError().text());
        ui->lb_statusConnect->setText("Отключено");
        ui->lb_statusConnect->setStyleSheet("color:red");
        msg->exec();
    }

}
void MainWindow::on_pb_clear_clicked()
{
    // 1. Отсоединяем модель от таблицы, чтобы данные исчезли с экрана
    ui->tableView->setModel(nullptr);

    // 2. Удаляем модели из памяти (чтобы не было утечек)
    if (tableModel) {
        delete tableModel;
        tableModel = nullptr;
    }
    if (queryModel) {
        delete queryModel;
        queryModel = nullptr;
    }
}



