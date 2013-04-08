Geometry pImage::minimumGeometry() {
  const QPixmap *pixmap = qtLabel->pixmap();
  if(pixmap && !pixmap->isNull())
    return { 0, 0, pixmap->width(), pixmap->height() };
  else
    return { 0, 0, 0, 0 };
}

void pImage::setImage(const image &image) {
  qtLabel->setPixmap(QPixmap());

  if(!image.empty()) {
    QPixmap pixmap = CreatePixmap(image);
    qtLabel->setPixmap(pixmap);
    qtLabel->setMinimumSize(pixmap.width(), pixmap.height());
  }
}

void pImage::constructor() {
  qtWidget = qtLabel = new QLabel;

  pWidget::synchronizeState();
}

void pImage::destructor() {
  delete qtLabel;
  qtWidget = qtLabel = 0;
}

void pImage::orphan() {
  destructor();
  constructor();
}
