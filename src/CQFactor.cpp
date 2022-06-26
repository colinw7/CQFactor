#include <CQFactor.h>

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

int
main(int argc, char **argv)
{
#ifdef USE_CQ_APP
  CQApp app(argc, argv);
#else
  QApplication app(argc, argv);
#endif

  auto *window = new CQFactor::Window;

  if (argc > 1) {
    int i = atoi(argv[1]);

    if (i > 0)
      window->setFactor(i);
  }

  window->show();

  app.exec();
}

//------

namespace CQFactor {

Window::
Window(QWidget *parent) :
 QWidget(parent)
{
  setObjectName("window");

  auto *layout = new QVBoxLayout(this);
  layout->setMargin(2); layout->setSpacing(2);

  app_ = new App;

  app_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  layout->addWidget(app_);

  auto *llayout = new QHBoxLayout;
  llayout->setMargin(2); llayout->setSpacing(2);

  edit_ = new QSpinBox;

  edit_->setRange(1, INT_MAX);
  edit_->setValue(4);

  edit_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  connect(edit_, SIGNAL(valueChanged(int)), this, SLOT(factorSlot()));

  llayout->addWidget(new QLabel("Number"));
  llayout->addWidget(edit_);

  auto *check = new QCheckBox("Debug");

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
  setObjectName("CQFactor");

  //setFocusPolicy(Qt::StrongFocus);

  setMinimumSize(QSize(400, 400));

  circleMgr_ = new AppCircleMgr(this);

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

  circleMgr_->setDebug(debug_);

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
  circleMgr_->reset();
}

void
App::
addDrawCircle(const QRectF &r, const QColor &pen, const QColor &brush)
{
  drawCircles_.emplace_back(r, pen, brush);

  auto &drawCircle = drawCircles_.back();

  if (oldInd_ < int(oldDrawCircles_.size())) {
    // update exiting old to existing circle
    auto &oldDrawCircle = oldDrawCircles_[size_t(oldInd_)];

    drawCircle.oldData = oldDrawCircle.data;

    ++oldInd_;
  }
  else {
    // add new circle at center
    double xc = width ()/2;
    double yc = height()/2;

    drawCircle.oldData.rect  = QRectF(xc - r.width()/2, yc - r.height()/2, r.width(), r.height());
    drawCircle.oldData.pen   = QColor(0, 0, 0, 0);
    drawCircle.oldData.brush = QColor(0, 0, 0, 0);
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
  auto factor = circleMgr_->factor();

  if (i != factor) {
    circleMgr_->setFactor(i);

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
  auto n1 = int(oldDrawCircles_.size());
  auto n2 = int(drawCircles_   .size());

  int nfade = n1 - n2;

  for (int i = 0; i < nfade; ++i) {
    DrawCircle drawCircle;

    drawCircle.oldData = oldDrawCircles_[size_t(n2 + i)].data;

    drawCircle.data.rect  = QRectF(width()/2, height()/2, 0.1, 0.1);
    drawCircle.data.pen   = QColor(0, 0, 0, 0);
    drawCircle.data.brush = QColor(0, 0, 0, 0);

    drawCircles_.push_back(drawCircle);
  }
}

void
App::
resetFade()
{
  for (auto &drawCircle : drawCircles_) {
    drawCircle.oldData.rect  = QRectF();
    drawCircle.oldData.pen   = drawCircle.data.pen;
    drawCircle.oldData.brush = drawCircle.data.brush;
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
  circleMgr_->calc();
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
generate()
{
  drawCircles_ .clear();
  debugCircles_.clear();

  circleMgr_->setCenter(CCircleFactor::Point(width()/2, height()/2));

  circleMgr_->generate(width(), height());
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
    auto c = interpPoint(from.center(), to.center(), f);

    auto s = interpSize(from.size(), to.size(), f);

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
      if (drawCircle.oldData.rect.isValid()) {
        drawCircle.oldData.rect  = interpRect (drawCircle.oldData.rect , drawCircle.data.rect , f);
        drawCircle.oldData.pen   = interpColor(drawCircle.oldData.pen  , drawCircle.data.pen  , f);
        drawCircle.oldData.brush = interpColor(drawCircle.oldData.brush, drawCircle.data.brush, f);
      }
    }
  }
  else {
    for (auto &drawCircle : drawCircles_) {
      drawCircle.oldData.rect = QRectF();
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
    if (drawCircle.oldData.rect.isValid()) {
      painter->setPen  (drawCircle.oldData.pen);
      painter->setBrush(drawCircle.oldData.brush);

      painter->drawEllipse(drawCircle.oldData.rect);
    }
    else {
      painter->setPen  (drawCircle.data.pen);
      painter->setBrush(drawCircle.data.brush);

      painter->drawEllipse(drawCircle.data.rect);
    }
  }

  for (const auto &debugCircle : debugCircles_) {
    painter->setPen  (debugCircle.data.pen);
    painter->setBrush(debugCircle.data.brush);

    painter->drawEllipse(debugCircle.data.rect);
  }

  //------

  // draw number and factors
  auto factor = circleMgr_->factor();

  auto factorStr = QString("%1").arg(factor);

  const auto &factors = circleMgr_->factors();

  auto nf = factors.size();

  QString factorsStr;

  if (nf > 1) {
    for (std::size_t i = 0; i < nf; ++i) {
      if (i > 0) factorsStr += "x";

      factorsStr += QString("%1").arg(factors[i]);
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

  painter->drawText(int(20 + td1), int(  fm.height() + 20 - fm.descent()), factorStr);
  painter->drawText(int(20 + td2), int(2*fm.height() + 20 - fm.descent()), factorsStr);
}

//---

}
