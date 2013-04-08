static LRESULT CALLBACK Image_windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  Object *object = (Object*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  if(object == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);
  if(!dynamic_cast<Image*>(object)) return DefWindowProc(hwnd, msg, wparam, lparam);
  Image &canvas = (Image&)*object;

  if(msg == WM_GETDLGCODE) {
    return DLGC_STATIC | DLGC_WANTCHARS;
  }

  if(msg == WM_PAINT) {
    canvas.p.paint();
    return TRUE;
  }

  return DefWindowProc(hwnd, msg, wparam, lparam);
}

Geometry pImage::minimumGeometry() {
  BITMAP bm = { 0 };
  if(hbitmap != 0 && GetObject(hbitmap, sizeof bm, &bm))
    return { 0, 0, bm.bmWidth, bm.bmHeight };

  return { 0, 0, 0, 0 };
}

void pImage::setImage(const image &image) {
  nall::image nallImage = image;
  nallImage.transform(0, 32, 255u << 24, 255u << 16, 255u << 8, 255u << 0);

  if(hbitmap) { DeleteObject(hbitmap); hbitmap = 0; }

  if(!image.empty())
    hbitmap = CreateBitmap(nallImage);

  InvalidateRect(hwnd, NULL, TRUE);
  UpdateWindow(hwnd);
}

void pImage::constructor() {
  hwnd = CreateWindow(L"phoenix_image", L"", WS_CHILD, 0, 0, 0, 0, parentWindow->p.hwnd, (HMENU)id, GetModuleHandle(0), 0);
  SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&img);
  synchronize();
}

void pImage::destructor() {
  DestroyWindow(hwnd);
}

void pImage::orphan() {
  destructor();
  constructor();
}

void pImage::paint() {
  RECT rc;
  GetClientRect(hwnd, &rc);
  unsigned width = 0, height = 0;

  BLENDFUNCTION blendfn = { AC_SRC_OVER, 0, 255, 0 };

  BITMAP bm;
  if(hbitmap != 0 && GetObject(hbitmap, sizeof bm, &bm)) {
    width = bm.bmWidth;
    height = bm.bmHeight;
  } else return;

  PAINTSTRUCT ps;
  BeginPaint(hwnd, &ps);
  HDC memdc = CreateCompatibleDC(ps.hdc);
  SelectObject(memdc, hbitmap);
  AlphaBlend(ps.hdc, 0, 0, width, height, memdc, 0, 0, width, height, blendfn);
  DeleteDC(memdc);
  EndPaint(hwnd, &ps);
}

