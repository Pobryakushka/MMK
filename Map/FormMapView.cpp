#include "FormMapView.h"
#include "ui_FormMapView.h"
#include <QBoxLayout>


//FormMapView::FormMapView(int xCoordMainWindow, int yCoordMainWindow, QWidget *parent) :
FormMapView::FormMapView(QWidget *parent) :
    QDialog(parent)/*, image("d:/Project/EMA_v1/images/Map/coldHalfYearMap.png")*/,
    ui(new Ui::FormMapView)
{
    ui->setupUi(this);

//    int X, Y;
//    X = QApplication::desktop()->size().width();
//    Y = QApplication::desktop()->size().height();
    //setBaseSize(X, Y);

//    QGraphicsView view;
//    view.setBackgroundBrush(QImage("d:/Project/EMA_v1/images/Map/coldHalfYearMap.png"));
//    view.setCacheMode(QGraphicsView::CacheBackground);

//    QMessageBox msgBox;
//    msgBox.setText(QString("X=%0, Y=%1").arg(xCoordMainWindow).arg(yCoordMainWindow));
//    msgBox.exec();

//    move(xCoordMainWindow, yCoordMainWindow);

    image = new QImage("d:/Project/EMA_v1/images/Map/coldHalfYearMap.png");

    imageLabel = new QLabel(this);
    imageLabel->setBackgroundRole(QPalette::Base);
    imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    imageLabel->setScaledContents(false);

//    scrollArea = new QScrollArea(this);
//    scrollArea->setBackgroundRole(QPalette::Dark);
//    scrollArea->setWidget(imageLabel);
    if (!layout())
        setLayout(new QHBoxLayout());
    layout()->addWidget(imageLabel);
//    imageLabel->setPixmap(QPixmap::fromImage(image.scaled(size(), Qt::IgnoreAspectRatio)));
//    setCentralWidget(scrollArea);
//    scrollArea->setWidgetResizable(true);

//    QImage image("d:/Project/EMA_v1/images/Map/coldHalfYearMap.png");
//    image = image.scaled(size(), Qt::KeepAspectRatio);
//    image = image.scaled(size(), Qt::IgnoreAspectRatio);
//    QPixmap pixmap(image);
//    pixmap.
//    imageLabel->setPixmap(QPixmap::fromImage(image));
//    imageLabel->adjustSize();



//    Q_ASSERT(imageLabel->pixmap());
//    imageLabel->resize(640, 480);

    QMessageBox msgBox2;
//    msgBox2.setText(QString("%").arg(imageLabel->pixmap()->size()));

    // msgBox2.setText(QString("%0x%1").arg(image->size().width()).arg(image->size().height()));
    // msgBox2.exec();

//    qDebug() << imageLabel->pixmap()->size();

//    imageLabel->resize(200, 100);
//    imageLabel->adjustSize();

//    imageLabel->setWidgetResizable(false);
//    imageLabel->resize(640, 480);

//    QImage img("/images/Map/coldHalfYearMap.png");                       // загружаем картинку
//    QPainter painter(this);                             // определяем объект painter, который обеспечивает рисование
//    painter.drawImage(0,0, img.scaled(this->size()));   // рисуем наше изображение от 0,0 и растягиваем по всему виджету
}

FormMapView::~FormMapView()
{
    delete ui;
    delete image;
}

void FormMapView::reject()
{
//    if (QMessageBox::question(this, "", "Вы уверенны?") == QMessageBox::Yes)
        QDialog::reject();
}

void FormMapView::resizeEvent(QResizeEvent *fmv)
{
    Q_UNUSED(fmv)
    imageLabel->setPixmap(QPixmap::fromImage(image->scaled(imageLabel->size(), Qt::KeepAspectRatio)));
//    imageLabel->setScaledContents(true);
    imageLabel->setMinimumWidth(160);
    imageLabel->setMinimumHeight(60);


    //    imageLabel->adjustSize();
//    FormMapView::resizeEvent(fmv);
}
