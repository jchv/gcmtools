Geometry pImage::minimumGeometry() {
  GdkPixbuf *pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(gtkWidget));
  
  if(!pixbuf)
    return { 0, 0, 0, 0 };
  
  int width = gdk_pixbuf_get_width(pixbuf);
  int height = gdk_pixbuf_get_height(pixbuf);
  
  return { 0, 0, width, height };
}

void pImage::setImage(const image &image) {
  if(!image.empty()) {
    GdkPixbuf *pixbuf = CreatePixbuf(image, false);
    gtk_image_set_from_pixbuf(GTK_IMAGE(gtkWidget), pixbuf);
  } else {
    gtk_image_clear(GTK_IMAGE(gtkWidget));
  }
}

void pImage::constructor() {
  gtkWidget = gtk_image_new();
}

void pImage::destructor() {
  gtk_widget_destroy(gtkWidget);
}

void pImage::orphan() {
  destructor();
  constructor();
}

