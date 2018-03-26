#include <CQFactor.h>
#include <CPrime.h>

#ifdef USE_CQ_APP
#include <CQApp.h>
#else
#include <QApplication>
#endif

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QTimer>
#include <QPainter>

#include <cmath>
#include <iostream>
#include <cassert>

int
main(int argc, char **argv)
{
#ifdef USE_CQ_APP
  CQApp app(argc, argv);
#else
  QApplication app(argc, argv);
#endif

  auto window = new CQFactor::Window;

  if (argc > 1) {
    int i = atoi(argv[1]);

    if (i > 0)
      window->setFactor(i);
  }

  window->show();

  app.exec();
}

//-------

namespace CQFactor {

Window::
Window(QWidget *parent) :
 QWidget(parent)
{
  auto layout = new QVBoxLayout(this);
  layout->setMargin(2); layout->setSpacing(2);

  app_ = new App;

  app_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  layout->addWidget(app_);

  auto llayout = new QHBoxLayout;
  llayout->setMargin(2); llayout->setSpacing(2);

  edit_ = new QSpinBox;

  edit_->setRange(1, INT_MAX);
  edit_->setValue(4);

  edit_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  connect(edit_, SIGNAL(valueChanged(int)), this, SLOT(factorSlot()));

  llayout->addWidget(new QLabel("Number"));
  llayout->addWidget(edit_);

  auto check = new QCheckBox("Debug");

  connect(check, SIGNAL(stateChanged(int)), this, SLOT(debugSlot(int)));

  llayout->addWidget(check);

  llayout->addStretch();

  layout->addLayout(llayout);

  connect(this, SIGNAL(factorEntered(int)), app_, SLOT(factorEntered(int)));

  factorSlot();

  app_->addTimer();
}

void
Window::
setFactor(int i)
{
  edit_->setValue(i);

  emit factorEntered(i);
}

void
Window::
factorSlot()
{
  auto i = edit_->value();

  emit factorEntered(i);
}

void
Window::
debugSlot(int value)
{
  app_->setDebug(value);
}

//-------

App::
App(QWidget *parent) :
 QWidget(parent)
{
  //setFocusPolicy(Qt::StrongFocus);

  setMinimumSize(QSize(400, 400));

  calc();
}

App::
~App()
{
  reset();
}

void
App::
setDebug(bool debug)
{
  debug_ = debug;

  applyFactor();
}

void
App::
addTimer()
{
  animateTimer_ = new QTimer;

  connect(animateTimer_, SIGNAL(timeout()), this, SLOT(animateSlot()));
}

void
App::
reset()
{
  delete circle_;

  circle_ = nullptr;
}

void
App::
addDrawCircle(const QRectF &r, const QColor &pen, const QColor &brush)
{
  drawCircles_.emplace_back(r, pen, brush);

  DrawCircle &drawCircle = drawCircles_.back();

  if (oldInd_ < int(oldDrawCircles_.size())) {
    DrawCircle &oldDrawCircle = oldDrawCircles_[oldInd_];

    drawCircle.oldRect  = oldDrawCircle.rect;
    drawCircle.oldPen   = oldDrawCircle.pen;
    drawCircle.oldBrush = oldDrawCircle.brush;

    ++oldInd_;
  }
  else {
    double xc = width ()/2;
    double yc = height()/2;

    drawCircle.oldRect  = QRectF(xc - r.width()/2, yc - r.height()/2, r.width(), r.height());
    drawCircle.oldPen   = QColor(0, 0, 0, 0);
    drawCircle.oldBrush = QColor(0, 0, 0, 0);
  }
}

void
App::
addDebugCircle(const QRectF &r, const QColor &pen, const QColor &brush)
{
  debugCircles_.emplace_back(r, pen, brush);
}

void
App::
factorEntered(int i)
{
  if (i != factor_) {
    factor_ = i;

    applyFactor();
  }
}

void
App::
applyFactor()
{
  calc();

  //---

  saveOld();

  generate();

  //---

  addFadeOut();

  //---

  animate();

  //---

  update();
}

void
App::
saveOld()
{
  oldDrawCircles_ = drawCircles_;
  oldInd_         = 0;
}

void
App::
addFadeOut()
{
  int n1 = oldDrawCircles_.size();
  int n2 = drawCircles_   .size();

  int nfade = n1 - n2;

  for (int i = 0; i < nfade; ++i) {
    DrawCircle drawCircle;

    drawCircle.oldRect  = oldDrawCircles_[n2 + i].rect;
    drawCircle.oldPen   = oldDrawCircles_[n2 + i].pen;
    drawCircle.oldBrush = oldDrawCircles_[n2 + i].brush;

    drawCircle.rect  = QRectF(width()/2, height()/2, 0.1, 0.1);
    drawCircle.pen   = QColor(0, 0, 0, 0);
    drawCircle.brush = QColor(0, 0, 0, 0);

    drawCircles_.push_back(drawCircle);
  }
}

void
App::
resetFade()
{
  for (auto &drawCircle : drawCircles_) {
    drawCircle.oldRect  = QRectF();
    drawCircle.oldPen   = drawCircle.pen;
    drawCircle.oldBrush = drawCircle.brush;
  }
}

void
App::
animate()
{
  if (animateTimer_) {
    animateCount_ = 0;

    animateTimer_->start(10);
  }
}

void
App::
calc()
{
  Circle::resetId();

  reset();

  factors_ = CPrime::factors(factor_);

  circle_ = new Circle(this);

  if (CPrime::isPrime(factor_))
    calcPrime(circle_, factor_);
  else
    calcFactors(circle_, factors_);

  circle_->place();

  circle_->fit();
}

void
App::
paintEvent(QPaintEvent *)
{
  QPainter painter(this);

  draw(&painter);
}

void
App::
resizeEvent(QResizeEvent *)
{
  applyFactor();

  resetFade();

  update();
}

void
App::
calcFactors(Circle *circle, const Factors &f)
{
  if (f.size() == 1) {
    calcPrime(circle, f[0]);
    return;
  }

  //------

  // split into first factor and list of remaining factors
  auto pf = f.begin();

  auto n1 = *pf++;

  Factors f1;

  std::copy(pf, f.end(), std::back_inserter(f1));

  //------

  // add n circles
  for (int i = 0; i < n1; ++i) {
    auto circle1 = new Circle(this, circle, i);

    circle->addCircle(circle1);

    calcFactors(circle1, f1);
  }
}

void
App::
calcPrime(Circle *circle, int n)
{
  assert(n > 0);

  // reserve n ids
  circle->setId(n);

  // add n points
  for (int i = 0; i < n; ++i)
    circle->addPoint();
}

void
App::
generate()
{
  drawCircles_ .clear();
  debugCircles_.clear();

  double xc = circle_->xc();
  double yc = circle_->yc();

  pos_  = QPointF(xc*width(), (1.0 - yc)*height());
  size_ = std::min(width(), height());

  circle_->generate(pos_, size_);
}

void
App::
animateSlot()
{
  animateStep();
}

void
App::
animateStep()
{
  auto interp = [](double from, double to, double d) {
    return from + (to - from)*d;
  };

  auto interpPoint = [&](const QPointF &from, const QPointF &to, double f) {
    return QPointF(interp(from.x(), to.x(), f), interp(from.y(), to.y(), f));
  };

  auto interpSize = [&](const QSizeF &from, const QSizeF &to, double f) {
    return QSizeF(interp(from.width (), to.width (), f),
                  interp(from.height(), to.height(), f));
  };

  auto interpRect = [&](const QRectF &from, const QRectF &to, double f) {
    QPointF c = interpPoint(from.center(), to.center(), f);

    QSizeF s = interpSize(from.size(), to.size(), f);

    return QRectF(c.x() - s.width()/2, c.y() - s.height()/2, s.width(), s.height());
  };

  auto interpColor = [&](const QColor &from, QColor &to, double f) {
    return QColor::fromRgbF(interp(from.redF  (), to.redF  (), f),
                            interp(from.greenF(), to.greenF(), f),
                            interp(from.blueF (), to.blueF (), f),
                            interp(from.alphaF(), to.alphaF(), f));
  };

  ++animateCount_;

  if (animateCount_ < animIterations()) {
    double f = 1.0/(animIterations() - animateCount_);

    for (auto &drawCircle : drawCircles_) {
      if (drawCircle.oldRect.isValid()) {
        drawCircle.oldRect  = interpRect (drawCircle.oldRect , drawCircle.rect , f);
        drawCircle.oldPen   = interpColor(drawCircle.oldPen  , drawCircle.pen  , f);
        drawCircle.oldBrush = interpColor(drawCircle.oldBrush, drawCircle.brush, f);
      }
    }
  }
  else {
    for (auto &drawCircle : drawCircles_) {
      drawCircle.oldRect = QRectF();
    }

    animateTimer_->stop();
  }

  update();
}

void
App::
draw(QPainter *painter)
{
  painter->setPen(QColor(0, 0, 0, 0));

  painter->setRenderHint(QPainter::Antialiasing, true);

  for (const auto &drawCircle : drawCircles_) {
    if (drawCircle.oldRect.isValid()) {
      painter->setPen  (drawCircle.oldPen);
      painter->setBrush(drawCircle.oldBrush);

      painter->drawEllipse(drawCircle.oldRect);
    }
    else {
      painter->setPen  (drawCircle.pen);
      painter->setBrush(drawCircle.brush);

      painter->drawEllipse(drawCircle.rect);
    }
  }

  for (const auto &debugCircle : debugCircles_) {
    painter->setPen  (debugCircle.pen);
    painter->setBrush(debugCircle.brush);

    painter->drawEllipse(debugCircle.rect);
  }

  //------

  // draw number and factors
  auto factorStr = QString("%1").arg(factor_);

  auto nf = factors_.size();

  QString factorsStr;

  if (nf > 1) {
    for (std::size_t i = 0; i < nf; ++i) {
      if (i > 0) factorsStr += "x";

      factorsStr += QString("%1").arg(factors_[i]);
    }
  }
  else {
    factorsStr = "Prime";
  }

  //--

  QFontMetricsF fm(font());

  double tw1 = fm.width(factorStr);
  double tw2 = fm.width(factorsStr);

  QRectF rect(20, 20, std::max(tw1, tw2), 2*fm.height());

  QColor c(255, 255, 255, 100);

  painter->fillRect(rect, QBrush(c));

  painter->setPen(QColor(0, 0, 0, 255));

  auto td1 = 0.0, td2 = 0.0;

  if (tw1 > tw2)
    td2 = (tw1 - tw2)/2.0;
  else
    td1 = (tw2 - tw1)/2.0;

  painter->drawText(20 + td1,   fm.height() + 20 - fm.descent(), factorStr);
  painter->drawText(20 + td2, 2*fm.height() + 20 - fm.descent(), factorsStr);
}

//------

Circle::
Circle(App *app) :
 app_(app)
{
}

Circle::
Circle(App *app, Circle *parent, std::size_t n) :
 app_(app), parent_(parent), n_(n)
{
}

Circle::
~Circle()
{
  for (auto &circle : circles_)
    delete circle;
}

void
Circle::
setId(std::size_t n)
{
  id_ = lastId();

  lastIdRef() += n;
}

void
Circle::
addCircle(Circle *circle)
{
  circles_.push_back(circle);
}

void
Circle::
addPoint()
{
  points_.emplace_back(0.0, 0.0);
}

void
Circle::
place()
{
  if (! circles_.empty()) {
    auto nc = circles_.size();

    double da = 2.0*M_PI/nc;

    // place child circles
    double a = a_;

    for (auto &circle : circles_) {
      if (size() == 2 && circle->size() == 2)
        circle->setA(a + M_PI/2.0);
      else
        circle->setA(a);

      circle->place();

      a += da;
    }

    // find minimum point distance for child circles
    double d = 1E50;

    for (auto &circle : circles_) {
      d = std::min(d, circle->closestPointDistance());
    }

    double rr = d/2.0;

    // place in circle (center (0.5, 0.5), radius 0.5)
    c_ = QPointF(0.5, 0.5);
    r_ = 0.5;

    for (;;) {
      a = a_;

      for (auto &circle : circles_) {
        double x1 = x() + r_*std::cos(a);
        double y1 = y() + r_*std::sin(a);

        circle->move(x1, y1);

        a += da;
      }

      double r1 = closestCircleCircleDistance()/2;

      double dr = fabs(r1 - rr);

      if (dr < 1E-3)
        break;

      if (r1 < rr)
        r_ += dr/2;
      else
        r_ -= dr/2;
    }
  }
  else {
    auto np = numPoints();

    c_ = QPointF(0.5, 0.5);
    r_ = 0.5;

    // place points in circle
    if (np > 1) {
      double a  = a_;
      double da = 2.0*M_PI/np;

      for (std::size_t i = 0; i < np; ++i) {
        double x1 = std::cos(a);
        double y1 = std::sin(a);

        setPoint(i, QPointF(x1, y1));

        a += da;
      }
    }
    else {
      setPoint(0, QPointF(0.0, 0.0));
    }
  }
}

#if 0
double
Circle::
calcR() const
{
  if (parent_) {
    if (parent_->size() == 2 && size() == 2)
      return parent_->calcR();
    else
      return 0.5*parent_->calcR();
  }
  else
    return 0.5;
}
#endif

void
Circle::
fit()
{
  // get all points
  Points points;

  getPoints(points);

  // calc closest centers and range
  auto np = points.size();

  double xmin = 0.5;
  double ymin = 0.5;
  double xmax = xmin;
  double ymax = ymin;

  double d = 2;

  for (std::size_t i = 0; i < np; ++i) {
    const QPointF &p1 = points[i];

    for (std::size_t j = i + 1; j < np; ++j) {
      assert(i != j);

      const QPointF &p2 = points[j];

      double dx = p1.x() - p2.x();
      double dy = p1.y() - p2.y();

      double d1 = dx*dx + dy*dy;

      if (d1 < d)
        d = d1;
    }

    xmin = std::min(xmin, p1.x());
    ymin = std::min(ymin, p1.y());
    xmax = std::max(xmax, p1.x());
    ymax = std::max(ymax, p1.y());
  }

  //---

  // use closest center to defined size so points don't touch
  double s = 0.0;

  if (d > 1E-6)
    s = sqrt(d);
  else
    s = 1.0/np;

  xmin -= s/2.0;
  ymin -= s/2.0;
  xmax += s/2.0;
  ymax += s/2.0;

  double xs = xmax - xmin;
  double ys = ymax - ymin;

  double maxS = std::max(xs, ys);

  app_->setS(s, maxS);

  xc_ = ((xmax + xmin)/2.0 - 0.5)/maxS + 0.5;
  yc_ = ((ymax + ymin)/2.0 - 0.5)/maxS + 0.5;

  //c_ += QPointF(xc_ - 0.5, yc_ - 0.5);

  //moveBy(0.5 - xc_, 0.5 - yc_);
}

double
Circle::
closestCircleCircleDistance() const
{
  // get all points
  CirclePoints points;

  getCirclePoints(points);

  // calc closest centers and range
  auto np = points.size();

  double d = 1E50;

  for (std::size_t i = 0; i < np; ++i) {
    const CirclePoint &p1 = points[i];

    for (std::size_t j = i + 1; j < np; ++j) {
      const CirclePoint &p2 = points[j];

      if (p1.circle == p2.circle) continue;

      double dx = p1.point.x() - p2.point.x();
      double dy = p1.point.y() - p2.point.y();

      double d1 = dx*dx + dy*dy;

      if (d1 < d)
        d = d1;
    }
  }

  return sqrt(d);
}

double
Circle::
closestPointDistance() const
{
  // get all points
  Points points;

  getPoints(points);

  // calc closest centers and range
  auto np = points.size();

  double d = 1E50;

  for (std::size_t i = 0; i < np; ++i) {
    const QPointF &p1 = points[i];

    for (std::size_t j = i + 1; j < np; ++j) {
      assert(i != j);

      const QPointF &p2 = points[j];

      double dx = p1.x() - p2.x();
      double dy = p1.y() - p2.y();

      double d1 = dx*dx + dy*dy;

      if (d1 < d)
        d = d1;
    }
  }

  return sqrt(d);
}

double
Circle::
closestSize() const
{
  double d = 1E50;

  if (! circles_.empty()) {
    auto nc = circles_.size();

    for (std::size_t i = 0; i < nc; ++i) {
      Circle *c1 = circles_[i];

      QPointF p1 = c1->center();

      for (std::size_t j = i + 1; j < nc; ++j) {
        assert(i != j);

        Circle *c2 = circles_[j];

        QPointF p2 = c2->center();

        double dx = p1.x() - p2.x();
        double dy = p1.y() - p2.y();

        double d1 = dx*dx + dy*dy;

        if (d1 < d)
          d = d1;
      }
    }
  }
  else {
    auto np = points_.size();

    for (std::size_t i = 0; i < np; ++i) {
      const QPointF &p1 = points_[i];

      for (std::size_t j = i + 1; j < np; ++j) {
        assert(i != j);

        const QPointF &p2 = points_[j];

        double dx = p1.x() - p2.x();
        double dy = p1.y() - p2.y();

        double d1 = dx*dx + dy*dy;

        if (d1 < d)
          d = d1;
      }
    }
  }

  d = sqrt(d);

  return d;
}

std::size_t
Circle::
size() const
{
  return std::max(circles_.size(), points_.size());
}

QPointF
Circle::
center() const
{
  return c_;
}

void
Circle::
getPoints(Points &points) const
{
  for (auto &circle : circles_)
    circle->getPoints(points);

  auto np = numPoints();

  for (std::size_t i = 0; i < np; ++i)
    points.push_back(getPoint(i));
}

void
Circle::
getCirclePoints(CirclePoints &points) const
{
  for (auto &circle : circles_)
    circle->getCirclePoints(points);

  auto np = numPoints();

  for (std::size_t i = 0; i < np; ++i)
    points.emplace_back(this, getPoint(i));
}

QPointF
Circle::
getPoint(int i) const
{
  return QPointF(x() + r_*points_[i].x(), y() + r_*points_[i].y());
}

void
Circle::
move(double x, double y)
{
  double dx = x - this->x();
  double dy = y - this->y();

  moveBy(dx, dy);
}

void
Circle::
moveBy(double dx, double dy)
{
  c_ += QPointF(dx, dy);

  for (auto &circle : circles_)
    circle->moveBy(dx, dy);
}

void
Circle::
generate(const QPointF &pos, double size)
{
  static double ps = 8;

  double size1 = size/app_->maxS();

  if (! circles_.empty()) {
    for (auto &circle : circles_)
      circle->generate(pos, size);
  }
  else {
    double s = 0.9*app_->s()*size1;

    // draw center
    if (app_->debug()) {
      double xc = (x() - 0.5)*size1 + pos.x();
      double yc = (y() - 0.5)*size1 + pos.y();

      app_->addDebugCircle(QRectF(xc - ps/2, yc - ps/2, ps, ps),
                           QColor(0, 0, 0, 0), QColor(0, 0, 0, 0.4*255));
    }

    // draw point circles
    auto np = numPoints();

    for (std::size_t i = 0; i < np; ++i) {
      QPointF p = getPoint(i);

      double x = (p.x() - 0.5)*size1 + pos.x();
      double y = (p.y() - 0.5)*size1 + pos.y();

      // draw point circle
      QColor c;

      c.setHsv((360.0*(id_ + i))/lastId(), 0.6*255, 0.6*255);

      app_->addDrawCircle(QRectF(x - s/2, y - s/2, s, s), QColor(0, 0, 0, 0), c);

      // draw point
      if (app_->debug()) {
        app_->addDebugCircle(QRectF(x - ps/2, y - ps/2, ps, ps),
                             QColor(0, 0, 0, 0), QColor(0, 0, 0, 255));
      }
    }
  }

  //------

  // draw bounding circle
  if (app_->debug()) {
    double s = r_*size1;

    double x = (this->x() - 0.5)*size1 + pos.x();
    double y = (this->y() - 0.5)*size1 + pos.y();

    app_->addDebugCircle(QRectF(x - s, y - s, 2*s, 2*s),
                         QColor(0, 0, 0, 0.4*255), QColor(0, 0, 0, 0));
  }
}

}
